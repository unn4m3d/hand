#include <Servo.h>
#include "servo.h"
#include "command.h"
#include "command_processor.h"

// Используемый последовательный порт
#define SERIAL Serial1

// Список машинок
hand::servo servos[] = {
    hand::servo(PA0, 128, 270), // Поворот
    hand::servo(PA1, 128, 270), // 1 сустав
    hand::servo(PA2, 128, 270), // 1 сустав
    hand::servo(PA3, 128, 270), // 2 сустав
    hand::servo(PA6, 128, 270), // 2 сустав
    hand::servo(PA7, 128, 270)  // клешня
};

const size_t servo_count = 6;

// Макрос для определения обработчиков команд
#define COMMAND_HANDLER(name) bool name (Stream& m_input, Print& m_reply)

/**
 * @brief Обработчик команды чтения
 *
 * Документацию к командам см. в command.h.
 * При обращении к несуществующей сервомашинке печатает в поток выходных данных строку "Invalid servo number N", где N - номер сервомашинки.
 * Иначе печатает "Success: N", где N - положение сервомашинки
 */
COMMAND_HANDLER(read_handler)
{
    int servo_number = m_input.parseInt();
    if(servo_number >= servo_count)
    {
        m_reply.print("Invalid servo number ");
        m_reply.println(servo_number);
        return false;
    }

    m_reply.print("Success: ");
    int arg = servos[servo_number].read();
    m_reply.println(arg);
    return true;
}


/**
 * @brief Обработчик команды записи
 *
 * Документацию к командам см. в command.h.
 * При обращении к несуществующей сервомашинке печатает в поток выходных данных строку "Invalid servo number N", где N - номер сервомашинки.
 * Иначе печатает "Success"
 */
COMMAND_HANDLER(write_handler)
{
    int servo_number = m_input.parseInt();
    int position = m_input.parseInt();

    if(servo_number >= servo_count)
    {
        m_reply.print("Invalid servo number ");
        m_reply.println(servo_number);
        return false;
    }

    servos[servo_number].write(position);
    m_reply.println("Success");

    return true;
}

/**
 * @brief Обработчик пинга
 *
 * Печатает "Pong" в поток вывода
 */
COMMAND_HANDLER(ping_handler)
{
    m_reply.println("Pong");
    return true;
}

/**
 * @brief Обработчик команды мультизаписи
 *
 * При обращении к несуществующей машинки прерывает выполнение команды и печатает "Invalid servo number N" в поток вывода.
 * При успешном выполнении печатает "Success"
 */
COMMAND_HANDLER(multi_write_handler)
{
    int servos_c = m_input.parseInt();
    for(int i = 0; i < servos_c; ++i)
    {
        int servo_number = m_input.parseInt();
        int position = m_input.parseInt();

        if(servo_number >= servo_count)
        {
            m_reply.print("Invalid servo number ");
            m_reply.println(servo_number);
            return false;
        }

        servos[servo_number].write(position);
    }
    m_reply.println("Success");

    return true;
}

// Высокоуровневые команды
/**
 * @brief Обработчик команды вращения
 */
COMMAND_HANDLER(rotate_handler)
{
    int angle = m_input.parseInt();
    bool relative = m_input.parseInt() != 0;

    if(relative)
        servos[0].write(servos[0].read() + angle);
    else
        servos[0].write(angle);

    m_reply.println("Success");
    return true;
}

/**
 * @brief Обработчик команды вытягивания руки
 */
COMMAND_HANDLER(extend_handler)
{
    int delta = m_input.parseInt();

    int joint_1 = servos[1].readDegrees(),
        joint_2 = servos[3].readDegrees();

    joint_1 += delta;
    joint_2 -= delta;

    servos[1].writeDegrees(joint_1);
    servos[2].writeDegrees(servos[2].getAngle() - joint_1);
    servos[3].writeDegrees(joint_2);
    servos[4].writeDegrees(servos[2].getAngle() - joint_2);

    m_reply.println("Success");
    return true;
}

/**
 * @brief Обработчик команды сжатия клешни
 */
COMMAND_HANDLER(capture_handler)
{
    int delta = m_input.parseInt();

    int joint_1 = servos[5].readDegrees() + delta;

    servos[5].writeDegrees(joint_1);

    m_reply.println("Success");
    return true;
}


hand::command_handler handlers[] = {
    NULL, //0
    &read_handler,//1
    &write_handler,//2
    &ping_handler,//3
    &multi_write_handler,//4
    &rotate_handler,//5
    &extend_handler,//6
    &capture_handler//7
};

const size_t handlers_count = 8;


void setup()
{
    SERIAL.begin(9600);
    SERIAL.println("Initialized");
}

/**
 * Обработчик поступающих команд
 */
hand::command_processor processor(handlers, handlers_count, SERIAL, SERIAL);

void loop()
{
    processor.try_process();
}
