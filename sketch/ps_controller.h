#pragma once

namespace hand
{
    enum ps2_button
    {
        PSB_NONE        = 0x0000,
        PSB_SELECT      = 0x0001,
        PSB_L3          = 0x0002,
        PSB_R3          = 0x0004,
        PSB_START       = 0x0008,
        PSB_PAD_UP      = 0x0010,
        PSB_PAD_RIGHT   = 0x0020,
        PSB_PAD_DOWN    = 0x0040,
        PSB_PAD_LEFT    = 0x0080,
        PSB_L2          = 0x0100,
        PSB_R2          = 0x0200,
        PSB_L1          = 0x0400,
        PSB_R1          = 0x0800,
        PSB_GREEN       = 0x1000,
        PSB_RED         = 0x2000,
        PSB_BLUE        = 0x4000,
        PSB_PINK        = 0x8000,
        PSB_TRIANGLE    = 0x1000,
        PSB_CIRCLE      = 0x2000,
        PSB_CROSS       = 0x4000,
        PSB_SQUARE      = 0x8000
    };

    enum ps2_analog
    {
        PSA_RX = 5,
        PSA_RY,
        PSA_LX,
        PSA_LY,
        PSA_PAD_RIGHT = 9,
        PSA_PAD_UP = 11,
        PSA_PAD_DOWN = 12,
        PSA_PAD_LEFT = 10,
        PSA_L2 = 19,
        PSA_R2 = 20,
        PSA_L1 = 17,
        PSA_R1 = 18,
        PSA_GREEN = 13,
        PSA_RED = 14,
        PSA_BLUE = 15,
        PSA_PINK = 16,
        PSA_TRIANGLE = 13,
        PSA_CIRCLE = 14,
        PSA_CROSS = 15,
        PSA_SQUARE = 16,
    };

    enum ps2_config_result
    {
        OK = 0,
        NO_CONTROLLER,
        NO_CONNECTION
    };

    const SPISettings gamepad_settings(500000, LSBFIRST, SPI_MODE2);

    typedef uint32_t register_t;

    template<typename T>
    inline bool check_bit(T& data, uint8_t idx)
    {
        return (data & 1 << idx);
    }

    template<typename T>
    inline void set_bit(T& data, uint8_t idx)
    {
        data |= (1 << idx);
    }

    template<typename T>
    inline void reset_bit(T& data, uint8_t idx)
    {
        data &= ~(1 << idx);
    }

    template<typename T>
    inline void toggle_bit(T& data, uint8_t idx)
    {
        data ^= (1 << idx);
    }

    const static uint8_t m_enter_cfg[5] = {0x01, 0x43, 0x00, 0x01, 0x00};
    const static uint8_t m_set_mode[] = {0x01, 0x44, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00};
    const static uint8_t m_set_bytes_large[] = {0x01,0x4F,0x00,0xFF,0xFF,0x03,0x00,0x00,0x00};
    const static uint8_t m_exit_cfg[] = {0x01,0x43,0x00,0x00,0x5A,0x5A,0x5A,0x5A,0x5A};
    const static uint8_t m_enable_rumble[] = {0x01,0x4D,0x00,0x00,0x01};
    const static uint8_t m_type_read[] = {0x01,0x45,0x00,0x5A,0x5A,0x5A,0x5A,0x5A,0x5A};

    class ps2_gamepad
    {
    private:
        void sendCommand(const uint8_t* buffer, uint8_t* output, uint16_t size)
        {
            SPI.beginTransaction(gamepad_settings);
            *m_ss_reg &= ~m_ss_mask;

            for(uint16_t i = 0; i < size; i++)
            {
                uint8_t temp = SPI.transfer(buffer[i]);
                if(output)
                    output[i] = temp;
            }

            *m_ss_reg |= m_ss_mask;
            SPI.endTransaction();
        }

        void sendCommand(const uint8_t* buffer, uint16_t size)
        {
            sendCommand(buffer, NULL, size);
        }

        void send(uint8_t* buffer, uint16_t size)
        {
            SPI.beginTransaction(gamepad_settings);
            *m_ss_reg &= ~m_ss_mask;

            SPI.transfer(buffer, size);

            *m_ss_reg |= m_ss_mask;
            SPI.endTransaction();
        }

        void reconfigure()
        {
            sendCommand(m_enter_cfg, sizeof(m_enter_cfg));
            sendCommand(m_set_mode, sizeof(m_set_mode));
            if(m_rumble)
                sendCommand(m_enable_rumble, sizeof(m_enable_rumble));
            if(m_pressures)
                sendCommand(m_set_bytes_large, sizeof(m_set_bytes_large));
            sendCommand(m_exit_cfg, sizeof(m_exit_cfg));
        }

        void readGamepad(uint8_t motor1, uint8_t motor2)
        {
            m_last_buttons = m_buttons;
            unsigned long last_delay = millis() - m_last_read;

            if(last_delay > 1500) reconfigure();
            if(last_delay < m_read_delay) delay(m_read_delay - last_delay);

            for(int i = 0; i < 21; i++)
            {
                m_buffer[i] = 0;
            }

            m_buffer[0] = 0x01;
            m_buffer[1] = 0x42;
            m_buffer[3] = motor1;
            m_buffer[4] = motor2;

            send(m_buffer, 9);

            if(m_buffer[1] = 0x79) // Full Data Return Mode
                send(m_buffer + 9, 12);

            m_buttons = *(uint16_t*)(m_buffer + 3);

            m_last_read = millis();
        }


    public:
        ps2_gamepad(uint8_t ss, bool rumble, bool pressures) :
            m_ss_pin(ss),
            m_rumble(rumble),
            m_pressures(pressures)
            {
                m_ss_mask = digitalPinToBitMask(ss);
                m_ss_reg = portOutputRegister(digitalPinToPort(ss));
                for(int i = 0; i < 21; i++)
                    m_buffer[i] = 0;
            }

        ps2_config_result configure()
        {
            m_last_read = millis();
            pinMode(m_ss_pin, OUTPUT);
            *m_ss_reg |= m_ss_mask;
            m_read_delay = 10; // TODO

            readGamepad(0, 0);

            switch (m_buffer[1])
            {
                case 0x41:
                case 0x73:
                case 0x79:
                    break;
                default:
                    return NO_CONTROLLER;
            }

            uint8_t type[sizeof(m_type_read)];

            sendCommand(m_enter_cfg, sizeof(m_enter_cfg));
            sendCommand(m_type_read, type, sizeof(m_type_read));

            m_controller_type = type[3];

            sendCommand(m_set_mode, sizeof(m_set_mode));

            if(m_rumble) sendCommand(m_enable_rumble, sizeof(m_enable_rumble));
            if(m_pressures) sendCommand(m_set_bytes_large, sizeof(m_set_bytes_large));

            sendCommand(m_exit_cfg, sizeof(m_exit_cfg));

            readGamepad(0, 0);

            return (m_buffer[1] == 0x73 && m_pressures) ? NO_CONNECTION : OK;
        }

        void update(uint8_t m1, uint8_t m2)
        {
            readGamepad(m1, m2);
        }

        bool button(ps2_button btn)
        {
            return ~m_buttons & btn;
        }

        bool changed()
        {
            return m_last_buttons ^ m_buttons;
        }

        bool changed(ps2_button btn)
        {
            return (m_last_buttons ^ m_buttons) & btn;
        }

        bool pressed(ps2_button btn)
        {
            return changed(btn) && button(btn);
        }

        bool released(ps2_button btn)
        {
             return changed(btn) && !button(btn);
        }

        uint8_t analog(ps2_analog data)
        {
            return m_buffer[data];
        }
    private:
        uint8_t m_ss_pin, m_ss_mask;
        volatile register_t *m_ss_reg;

        unsigned long m_last_read;
        bool m_rumble, m_pressures;
        uint8_t m_read_delay;
        uint8_t m_buffer[21], m_controller_type;
        uint16_t m_buttons, m_last_buttons;
    };
}