#include <fpp-pch.h>

#include "I2C_BitBang.h"

#if defined(PLATFORM_BBB) || defined(PLATFORM_BB64)
#include "util/BBBUtils.h"
#define PLAT_GPIO_CLASS BBBPinProvider
#elif defined(PLATFORM_PI)
#include "util/PiGPIOUtils.h"
#define PLAT_GPIO_CLASS PiGPIOPinProvider
#else
#include "util/TmpFileGPIO.h"
#define PLAT_GPIO_CLASS TmpFilePinProvider
#endif

#include <chrono>
#include <thread>

constexpr const char* PINMODE = "gpio";
constexpr bool OUTPUT = true;
constexpr bool INPUT = false;

constexpr bool HIGH = true;
constexpr bool LOW = false;

// Uses MRDS192 chip
// https://pira.cz/rds/mrds192.pdf
// RDS bus speed: 0 - 600 Hz
// 1.667ms Bit duration
// https://lucidar.me/en/serialib/most-used-baud-rates-table/

I2C_BitBang::I2C_BitBang(std::string sda, std::string scl) :
        started (false)
{
    PinCapabilities::InitGPIO("FPPEDMRDS", new PLAT_GPIO_CLASS());
    sda_pin = &PinCapabilities::getPinByName(sda);
    scl_pin = &PinCapabilities::getPinByName(scl);
    sda_pin->configPin(PINMODE, INPUT);
    scl_pin->configPin(PINMODE, INPUT);
}

bool I2C_BitBang::read_SCL() // Set SCL as input and return current level of line, 0 or 1
{
    scl_pin->configPin(PINMODE, INPUT);
    return scl_pin->getValue();
}

bool I2C_BitBang::read_SDA() // Set SDA as input and return current level of line, 0 or 1
{
    sda_pin->configPin(PINMODE, INPUT);
    return sda_pin->getValue();
}

void I2C_BitBang::set_SCL()   // Do not drive SCL (set pin high-impedance)
{
    scl_pin->configPin(PINMODE, INPUT);
}

void I2C_BitBang::clear_SCL() // Actively drive SCL signal low
{
    scl_pin->configPin(PINMODE, OUTPUT);
    scl_pin->setValue(LOW);
}

void I2C_BitBang::set_SDA()   // Do not drive SDA (set pin high-impedance)
{
    sda_pin->configPin(PINMODE, INPUT);
}

void I2C_BitBang::clear_SDA() // Actively drive SDA signal low
{
    sda_pin->configPin(PINMODE, OUTPUT);
    sda_pin->setValue(LOW);
}

void I2C_BitBang::arbitration_lost(std::string method)
{
    throw std::runtime_error("error: " + method);
}

void I2C_BitBang::i2c_start_cond()
{
  if (started) { 
    // if started, do a restart condition
    // set SDA to 1
    set_SDA();
    i2c_delay();
    set_SCL();
    while (read_SCL() == 0) { // Clock stretching
      // You should add timeout to this loop
      i2c_wait();
    }

    // Repeated start setup time, minimum 4.7us
    i2c_delay();
  }

  if (read_SDA() == 0) {
    arbitration_lost(__func__);
  }

  // SCL is high, set SDA from 1 to 0.
  clear_SDA();
  i2c_delay();
  clear_SCL();
  started = true;
}

void I2C_BitBang::i2c_stop_cond()
{
  // set SDA to 0
  clear_SDA();
  i2c_delay();

  set_SCL();
  // Clock stretching
  while (read_SCL() == 0) {
    // add timeout to this loop.
    i2c_wait();
  }

  // Stop bit setup time, minimum 4us
  i2c_delay();

  // SCL is high, set SDA from 0 to 1
  set_SDA();
  i2c_delay();

  if (read_SDA() == 0) {
    arbitration_lost(__func__);
  }

  started = false;
}

// Write a bit to I2C bus
void I2C_BitBang::i2c_write_bit(bool bit)
{
  if (bit) {
    set_SDA();
  } else {
    clear_SDA();
  }

  // SDA change propagation delay
  i2c_delay();

  // Set SCL high to indicate a new valid SDA value is available
  set_SCL();

  // Wait for SDA value to be read by target, minimum of 4us for standard mode
  i2c_delay();

  while (read_SCL() == 0) { // Clock stretching
    // You should add timeout to this loop
    i2c_wait();
  }

  // SCL is high, now data is valid
  // If SDA is high, check that nobody else is driving SDA
  if (bit && (read_SDA() == 0)) {
    arbitration_lost(__func__);
  }

  // Clear the SCL to low in preparation for next change
  clear_SCL();
}

// Read a bit from I2C bus
bool I2C_BitBang::i2c_read_bit()
{
  bool bit;

  // Let the target drive data
  set_SDA();

  // Wait for SDA value to be written by target, minimum of 4us for standard mode
  i2c_delay();

  // Set SCL high to indicate a new valid SDA value is available
  set_SCL();

  while (read_SCL() == 0) { // Clock stretching
    // You should add timeout to this loop
    i2c_wait();
  }

  // Wait for SDA value to be written by target, minimum of 4us for standard mode
  i2c_delay();

  // SCL is high, read out bit
  bit = read_SDA();

  // Set SCL low in preparation for next operation
  clear_SCL();

  return bit;
}

// Write a byte to I2C bus. Return 0 if ack by the target.
bool I2C_BitBang::i2c_write_byte(bool send_start,
                    bool send_stop,
                    unsigned char byte)
{
  unsigned bit;
  bool     nack;

  if (send_start) {
    i2c_start_cond();
  }

  for (bit = 0; bit < 8; ++bit) {
    i2c_write_bit((byte & 0x80) != 0);
    byte <<= 1;
  }

  nack = i2c_read_bit();

  if (send_stop) {
    i2c_stop_cond();
  }

  return nack;
}

// Read a byte from I2C bus
unsigned char I2C_BitBang::i2c_read_byte(bool nack, bool send_stop)
{
  unsigned char byte = 0;
  unsigned char bit;

  for (bit = 0; bit < 8; ++bit) {
    byte = (byte << 1) | i2c_read_bit();
  }

  i2c_write_bit(nack);

  if (send_stop) {
    i2c_stop_cond();
  }

  return byte;
}


void I2C_BitBang::i2c_delay()
{ 
    std::this_thread::sleep_for( std::chrono::milliseconds(2) ); //500hz
}

void I2C_BitBang::i2c_wait()
{ 
  std::this_thread::sleep_for( std::chrono::nanoseconds(1) );
}

int32_t I2C_BitBang::i2c_write_block_data(uint8_t i2c_address, uint8_t command, uint8_t length,
        const uint8_t * values)
{
    // 7 bit address + 1 bit read/write
    uint8_t address = (i2c_address << 1) | 0;

    if (!i2c_write_byte(true, false, address)) {
        if (!i2c_write_byte(false, false, command)) {
            bool errors {false};
            for (size_t i = 0; i < length; i++)
            {
                if (!errors) {
                    errors = i2c_write_byte(false, false, values[i]);
                }
            }

            i2c_stop_cond();

            if (!errors) {  
              return 0;
            }
        } else {
           i2c_stop_cond();
        }
    } else {   
       i2c_stop_cond();
    }

    return -1;
}


