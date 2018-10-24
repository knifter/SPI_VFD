#ifndef SPI_VFD_h
#define SPI_VFD_h

#include <Arduino.h>
#include <SPI.h>
#include <inttypes.h>
#include <Print.h>

// VFD Module 20T202DA2JA
// Controller: µ PD16314

class SPI_VFD : public Print 
{
    public:
        SPI_VFD(SPIClass& spi, uint8_t cols = 20, uint8_t rows = 2);

        void begin();

        void clear();
        void home();

        void setBrightness(uint8_t brightness);
        uint8_t getBrightness();

        void displayOn(bool = true);
        void blink(bool = true);
        void cursor(bool = true);
        void scrollDisplayLeft();
        void scrollDisplayRight();
        void leftToRight();
        void rightToLeft();
        void autoscroll(bool on = true);

        void createChar(uint8_t, uint8_t[]);
        void setCursor(uint8_t, uint8_t); 
        virtual size_t write(uint8_t);

    private:
        inline void command(uint8_t);

        SPIClass& _spi;

        uint8_t _displayfunction;
        uint8_t _displaycontrol;
        uint8_t _displaymode;

        uint8_t _numlines = 2;
        uint8_t _currline = 0;
};

#endif