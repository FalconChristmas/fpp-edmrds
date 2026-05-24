#include <fpp-pch.h>

#include <string>
#include <vector>

#include <unistd.h>
#include <termios.h>

#include "mediadetails.h"
#include "commands/Commands.h"
#include "common.h"
#include "settings.h"
#include "Plugin.h"
#include "log.h"

#include "I2C_BitBang.h"

static std::string padToNearest(std::string s, int l) {
    if (!s.empty()) {
        int n = 0;
        while (n < s.size()) {
            n += l;
        }
        if (n != s.size()) {
            size_t a = n - s.size();
            s.append(a, ' ');
        }
    }
    return s;
}
static void padTo(std::string &s, int l) {
    size_t n = l - s.size();
    if (n) {
        s.append(n, ' ');
    }
}

class FPPEDMRDSPlugin : public FPPPlugin {
public:
    bool enabled = true;
    bool rdsEnabled = false;
    FPPEDMRDSPlugin() : FPPPlugin("fpp-edmrds") {
        setDefaultSettings();
        startRDS();
       
    }
    virtual ~FPPEDMRDSPlugin() {

    }

    bool startRDS() {
        
        i2c_bb = std::make_unique<I2C_BitBang>("23","24");
        return true;
    }

    void initRDS() {
        LogInfo(VB_PLUGIN, "Enabling RDS\n");

        formatAndSendText(settings["StationText"], "", "", true);
        formatAndSendText(settings["RDSTextText"], "", "", false);
    }
    
    
    void stopRDS() {
        
    }

void setRDSBuffer(uint8_t address, const std::string &station) {
    if (lastRDS == station) {
        return;
    }
    lastRDS = station;
    uint8_t buf[64];
    memset(buf, ' ', 64);
        
    int sl = station.size();
    if (sl > 64) {
        sl = 64;
    }
    for (int x = 0; x < sl; x++) {
        buf[x] = station[x];
    }
    for (int x = (sl-1); x > 0; --x) {
        if (buf[x] == ' ') {
            sl--;
        } else {
            break;
        }
    }

    std::vector<uint8_t> resp(6);
    if (station.size() != 0) {
		i2c_bb->i2c_write_block_data(0xD6, address, sizeof(buf), &buf[0]);
    }
}
    
    void formatAndSendText(const std::string &text, const std::string &artist, const std::string &title, bool station) {
        std::string output;
        
        int artistIdx = -1;
        int titleIdx = -1;

        for (int x = 0; x < text.length(); x++) {
            if (text[x] == '[') {
                if (artist == "" && title == "") {
                    while (text[x] != ']' && x < text.length()) {
                        x++;
                    }
                }
            } else if (text[x] == ']') {
                //nothing
            } else if (text[x] == '{') {
                const static std::string ARTIST = "{Artist}";
                const static std::string TITLE = "{Title}";
                std::string subs = text.substr(x);
                if (subs.rfind(ARTIST) == 0) {
                    artistIdx = output.length();
                    x += ARTIST.length() - 1;
                    output += artist;
                } else if (subs.rfind(TITLE) == 0) {
                    titleIdx = output.length();
                    x += TITLE.length() - 1;
                    output += title;
                } else {
                    output += text[x];
                }
            } else {
                output += text[x];
            }
        }
        if (station) {
            LogDebug(VB_PLUGIN, "Setting RDS Station text to \"%s\"\n", output.c_str());
            while (output.size()) {
                if (output.size() <= 8) {
                    padTo(output, 8);
                    output.clear();
                } else {
                    std::string lft = output.substr(0, 8);
                    padTo(lft, 8);
                    output = output.substr(8);
                }
            }
            setRDSBuffer(0x02, output);
        } else {
            LogDebug(VB_PLUGIN, "Setting RDS text to \"%s\"\n", output.c_str());
            setRDSBuffer(0x20, output);
        }
    }
    

    virtual void playlistCallback(const Json::Value &playlist, const std::string &action, const std::string &section, int item) {
        if (action == "stop" && rdsEnabled) {
            formatAndSendText(settings["StationText"], "", "", true);
            formatAndSendText(settings["RDSTextText"], "", "", false);
        }
        if (settings["Start"] == "PlaylistStart" && action == "start") {
            startRDS();
        } else if (settings["Stop"] == "PlaylistStop" && action == "stop") {
            stopRDS();
        }
        
    }
    virtual void mediaCallback(const Json::Value &playlist, const MediaDetails &mediaDetails) {
        std::string title = mediaDetails.title;
        std::string artist = mediaDetails.artist;
        //std::string album = mediaDetails.album;        
        formatAndSendText(settings["StationText"], artist, title, true);
        formatAndSendText(settings["RDSTextText"], artist, title, false);
    }
    
    
    void setDefaultSettings() {

        setIfNotFound("StationText", "Merry   Christ- mas", true);
        setIfNotFound("RDSTextText", "[{Artist} - {Title}]", true);
    }
    void setIfNotFound(const std::string &s, const std::string &v, bool emptyAllowed = false) {
        if (settings.find(s) == settings.end()) {
            settings[s] = v;
        } else if (!emptyAllowed && settings[s] == "") {
            settings[s] = v;
        }
        LogDebug(VB_PLUGIN, "Setting \"%s\": \"%s\"\n", s.c_str(), settings[s].c_str());
    }
    
    std::unique_ptr<I2C_BitBang> i2c_bb{nullptr};
    std::string lastRDS;
};


extern "C" {
    FPPPlugin *createPlugin() {
        return new FPPEDMRDSPlugin();
    }
}
