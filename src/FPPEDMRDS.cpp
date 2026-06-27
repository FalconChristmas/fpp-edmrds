/*
 * fpp-edmrds: drive a PIRA MRDS192 RDS encoder (as used by the EDM FM
 * transmitters) over a bit-banged, very-slow (~600 baud) open-drain I2C bus.
 *
 * This replaces the original Python + pigpio implementation. pigpio is gone on
 * FPP10 / Debian trixie and is Pi-only; this C++ version drives the bus through
 * FPP's PinCapabilities GPIO layer (open-drain "gpio_od" mode), so it works on
 * the Pi (incl. Pi5/RP1) and the BeagleBones using the same code.
 *
 * The MRDS192 is an I2C slave: write address 0xD6, read address 0xD7. The
 * commands used here: reg 0x02 = PS (station name, 8 chars), reg 0x20 = RT
 * (radiotext, 64 chars), reg 0x76 = dynamic-PS enable, reg 0x71 = save to EEPROM
 * (value 0x45). (pira.cz/rds/mrds192.pdf, edmdesign.com/docs/EDM-TX-RDS.pdf)
 */
#include <fpp-pch.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "Plugin.h"
#include "commands/Commands.h"
#include "log.h"
#include "mediadetails.h"
#include "util/GPIOUtils.h"

#define MRDS192_WRITE 0xD6
#define MRDS192_READ  0xD7
#define MRDS_REG_PS   0x02
#define MRDS_REG_RT   0x20
#define MRDS_REG_DPS  0x76
#define MRDS_REG_SAVE 0x71
#define MRDS_SAVE_KEY 0x45

// ---------------------------------------------------------------------------
// Bit-banged open-drain I2C master over two PinCapabilities lines.
// At ~600 baud (833us half-bit) userspace timing has huge margin, so plain
// usleep between transitions is reliable.
// ---------------------------------------------------------------------------
class BitBangI2C {
public:
    bool init(const std::string& sclPin, const std::string& sdaPin, int baud) {
        scl = lookupPin(sclPin);
        sda = lookupPin(sdaPin);
        if (scl == nullptr || sda == nullptr) {
            LogErr(VB_PLUGIN, "EDMRDS: could not resolve I2C pins SCL='%s' SDA='%s'\n",
                   sclPin.c_str(), sdaPin.c_str());
            return false;
        }
        if (baud < 1) {
            baud = 600;
        }
        halfDelayUs = 500000 / baud;
        // Mux both lines once as open-drain GPIO with pull-up. Idle = released
        // (high). No per-bit reconfigure or direction flipping after this.
        scl->configPin("gpio_od", false, "EDMRDS-SCL");
        sda->configPin("gpio_od", false, "EDMRDS-SDA");
        scl->setValue(true);
        sda->setValue(true);
        delayHalf();
        return true;
    }

    void start() {
        setSDA(true);
        setSCL(true);
        delayHalf();
        setSDA(false);  // SDA falls while SCL high
        delayHalf();
        setSCL(false);
        delayHalf();
    }
    void stop() {
        setSCL(false);
        setSDA(false);
        delayHalf();
        setSCL(true);
        delayHalf();
        setSDA(true);   // SDA rises while SCL high
        delayHalf();
    }
    // Returns true if the slave ACKed.
    bool writeByte(uint8_t b) {
        for (int i = 0; i < 8; i++) {
            setSDA((b & 0x80) != 0);
            b <<= 1;
            delayHalf();
            setSCL(true);
            delayHalf();
            setSCL(false);
        }
        setSDA(true);   // release for the slave's ACK
        delayHalf();
        setSCL(true);
        delayHalf();
        bool ack = !getSDA();  // slave pulls low = ACK
        setSCL(false);
        delayHalf();
        return ack;
    }
    uint8_t readByte(bool sendAck) {
        uint8_t b = 0;
        setSDA(true);   // release so the slave can drive
        for (int i = 0; i < 8; i++) {
            delayHalf();
            setSCL(true);
            delayHalf();
            b = (b << 1) | (getSDA() ? 1 : 0);
            setSCL(false);
        }
        setSDA(!sendAck);  // ACK = drive low, NACK = release high
        delayHalf();
        setSCL(true);
        delayHalf();
        setSCL(false);
        delayHalf();
        setSDA(true);
        return b;
    }

    // Write a register + payload to the MRDS192 (no ACK enforcement -- the
    // device is trusted, matching the original implementation).
    void writeCommand(uint8_t reg, const std::string& data) {
        start();
        writeByte(MRDS192_WRITE);
        writeByte(reg);
        for (unsigned char c : data) {
            writeByte(c);
        }
        stop();
    }
    void writeCommand(uint8_t reg, uint8_t value) {
        start();
        writeByte(MRDS192_WRITE);
        writeByte(reg);
        writeByte(value);
        stop();
    }

private:
    static const PinCapabilities* lookupPin(const std::string& s) {
        if (!s.empty() && std::all_of(s.begin(), s.end(), ::isdigit)) {
            return PinCapabilities::getPinByGPIO(0, std::stoi(s)).ptr();
        }
        return PinCapabilities::getPinByName(s).ptr();
    }
    void delayHalf() { std::this_thread::sleep_for(std::chrono::microseconds(halfDelayUs)); }
    void setSCL(bool v) { scl->setValue(v); }
    void setSDA(bool v) { sda->setValue(v); }
    bool getSDA() { return sda->getValue(); }

    const PinCapabilities* scl = nullptr;
    const PinCapabilities* sda = nullptr;
    int halfDelayUs = 833;  // ~600 baud
};

// ---------------------------------------------------------------------------
// Plugin
// ---------------------------------------------------------------------------
class FPPEDMRDSPlugin;

class EDMRDSStationNameCommand : public Command {
public:
    EDMRDSStationNameCommand(FPPEDMRDSPlugin* p);
    virtual ~EDMRDSStationNameCommand() {}
    virtual std::unique_ptr<Result> run(const std::vector<std::string>& args) override;
    FPPEDMRDSPlugin* plugin;
};
class EDMRDSInstallCommand : public Command {
public:
    EDMRDSInstallCommand(FPPEDMRDSPlugin* p);
    virtual ~EDMRDSInstallCommand() {}
    virtual std::unique_ptr<Result> run(const std::vector<std::string>& args) override;
    FPPEDMRDSPlugin* plugin;
};

class FPPEDMRDSPlugin : public FPPPlugins::Plugin, public FPPPlugins::PlaylistEventPlugin {
public:
    FPPEDMRDSPlugin() :
        FPPPlugins::Plugin("fpp-edmrds"),
        FPPPlugins::PlaylistEventPlugin() {
        setDefaultSettings();
        enabled = settings["Enabled"] == "1";
        if (!enabled) {
            LogInfo(VB_PLUGIN, "EDMRDS: plugin disabled\n");
            return;
        }
        i2cReady = i2c.init(settings["SCLPin"], settings["SDAPin"], std::stoi(settings["Baud"]));
        if (!i2cReady) {
            return;
        }
        if (settings["StationName"] != "") {
            i2c.writeCommand(MRDS_REG_PS, pad(settings["StationName"], 8));
        }
        running = true;
        worker = new std::thread([this]() { workerLoop(); });

        CommandManager::INSTANCE.addCommand(new EDMRDSStationNameCommand(this));
        CommandManager::INSTANCE.addCommand(new EDMRDSInstallCommand(this));
    }
    virtual ~FPPEDMRDSPlugin() {
        running = false;
        condition.notify_all();
        if (worker) {
            worker->join();
            delete worker;
        }
    }

    // now-playing -> radiotext
    virtual void mediaCallback(const Json::Value& playlist, const MediaDetails& md) override {
        if (!enabled || !i2cReady) {
            return;
        }
        std::string rt = md.title;
        if (!md.artist.empty()) {
            rt = md.artist + " - " + md.title;
        }
        queueRadioText(rt);
    }

    void setStationName(const std::string& name) {
        if (i2cReady) {
            i2c.writeCommand(MRDS_REG_PS, pad(name, 8));
        }
    }
    // Turn off dynamic PS (EDM ships it on) and save settings to EEPROM.
    void runInstall() {
        if (i2cReady) {
            i2c.writeCommand(MRDS_REG_DPS, (uint8_t)0x00);
            i2c.writeCommand(MRDS_REG_SAVE, (uint8_t)MRDS_SAVE_KEY);
        }
    }

private:
    void queueRadioText(const std::string& rt) {
        {
            std::unique_lock<std::mutex> lk(lock);
            pendingRT = rt;
            hasPending = true;
        }
        condition.notify_all();
    }
    // The I2C write of a 64-char radiotext takes ~1s at 600 baud, so do it off
    // the media-callback thread. Only the most recent request is sent.
    void workerLoop() {
        while (running) {
            std::string rt;
            {
                std::unique_lock<std::mutex> lk(lock);
                condition.wait(lk, [this]() { return hasPending || !running; });
                if (!running) {
                    break;
                }
                rt = pendingRT;
                hasPending = false;
            }
            i2c.writeCommand(MRDS_REG_RT, pad(rt, 64));
        }
    }
    static std::string pad(const std::string& s, size_t len) {
        std::string out = s.substr(0, len);
        out.resize(len, ' ');
        return out;
    }
    void setDefaultSettings() {
        setIfNotFound("Enabled", "0");
        setIfNotFound("SCLPin", "23");  // BCM GPIO23 (Pi header pin 16) by default
        setIfNotFound("SDAPin", "24");  // BCM GPIO24 (Pi header pin 18)
        setIfNotFound("Baud", "600");
        setIfNotFound("StationName", "", true);
    }
    void setIfNotFound(const std::string& s, const std::string& v, bool emptyAllowed = false) {
        if (settings.find(s) == settings.end()) {
            settings[s] = v;
        } else if (!emptyAllowed && settings[s] == "") {
            settings[s] = v;
        }
    }

    BitBangI2C i2c;
    bool enabled = false;
    bool i2cReady = false;

    std::thread* worker = nullptr;
    std::mutex lock;
    std::condition_variable condition;
    std::string pendingRT;
    bool hasPending = false;
    volatile bool running = false;
};

EDMRDSStationNameCommand::EDMRDSStationNameCommand(FPPEDMRDSPlugin* p) :
    Command("RDS Set Station Name", "EDMRDS - set the RDS station name (PS)"), plugin(p) {
    args.push_back(CommandArg("name", "string", "Station Name (8 chars)"));
}
std::unique_ptr<Command::Result> EDMRDSStationNameCommand::run(const std::vector<std::string>& args) {
    if (!args.empty()) {
        plugin->setStationName(args[0]);
    }
    return std::make_unique<Command::Result>("OK");
}

EDMRDSInstallCommand::EDMRDSInstallCommand(FPPEDMRDSPlugin* p) :
    Command("RDS Install", "EDMRDS - disable dynamic PS and save to EEPROM"), plugin(p) {}
std::unique_ptr<Command::Result> EDMRDSInstallCommand::run(const std::vector<std::string>& args) {
    plugin->runInstall();
    return std::make_unique<Command::Result>("OK");
}

extern "C" {
FPPPlugins::Plugin* createPlugin() {
    return new FPPEDMRDSPlugin();
}
}
