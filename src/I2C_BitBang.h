#pragma once

#include <cstdint>
#include <string>

#include "util/GPIOUtils.h"

class I2C_BitBang
{
    public:
        I2C_BitBang(std::string sda, std::string scl);

        int32_t i2c_write_block_data(uint8_t i2c_address, uint8_t command, uint8_t length, const uint8_t * values);

    // http://en.wikipedia.org/wiki/I%C2%B2C
    // https://en.wikipedia.org/wiki/I%C2%B2C#Example_of_bit-banging_the_I.C2.B2C_master_protocol
    private:
        bool read_SCL(); // Set SCL as input and return current level of line, 0 or 1
        bool read_SDA(); // Set SDA as input and return current level of line, 0 or 1
        void set_SCL();   // Do not drive SCL (set pin high-impedance)
        void clear_SCL(); // Actively drive SCL signal low
        void set_SDA();   // Do not drive SDA (set pin high-impedance)
        void clear_SDA(); // Actively drive SDA signal low

        void arbitration_lost(std::string method);

        void i2c_delay();
        void i2c_wait();

        void i2c_start_cond();
        void i2c_stop_cond();
        void i2c_write_bit(bool bit);
        bool i2c_read_bit();
        bool i2c_write_byte(bool send_start, bool send_stop, uint8_t byte);
        uint8_t i2c_read_byte(bool nack, bool send_stop);


        const PinCapabilities* sda_pin{nullptr};
        const PinCapabilities* scl_pin{nullptr};
        bool started = false; // global data

};