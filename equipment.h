/**
 * @file equipment.h
 * @brief Класс для представления единицы строительной техники
 * @author Дищенко Дмитрий Игоревич МГСУ ИЦТМС 2-1
 * 
 * Выполнение пунктов ТЗ:
 * - п.3: Класс содержит 8 параметров строительной техники
 * - п.5: Полная документация с помощью Doxygen
 * - п.14: Использование подходящих контейнеров Qt/STL
 */

#ifndef EQUIPMENT_H
#define EQUIPMENT_H

#include <QObject>
#include <QString>
#include <QDate>

/**
 * @class Equipment
 * @brief Класс единицы строительной техники
 *
 * Содержит 8 параметров техники согласно ТЗ (п.3):
 * 1. ID - уникальный идентификатор
 * 2. Model - модель техники
 * 3. Type - тип техники (экскаватор, бульдозер и т.д.)
 * 4. Year - год выпуска
 * 5. Registration Number - регистрационный номер
 * 6. Operating Hours - наработка в часах
 * 7. Condition - техническое состояние
 * 8. Last Maintenance - дата последнего ТО
 * 
 * Выполнение пунктов ТЗ:
 * - п.3: 8 параметров для каждой записи
 * - п.11: Валидация данных
 * - п.14: Использование контейнеров Qt
 */
class Equipment
{
public:
    /**
     * @brief Конструктор класса Equipment
     * @param id Уникальный идентификатор
     * @param model Модель техники
     * @param type Тип техники
     * @param year Год выпуска
     * @param regNumber Регистрационный номер
     * @param operatingHours Наработка в часах
     * @param condition Техническое состояние
     * @param lastMaintenance Дата последнего ТО
     */
    Equipment(int id = 0,
              const QString& model = "",
              const QString& type = "",
              int year = 2000,
              const QString& regNumber = "",
              double operatingHours = 0.0,
              const QString& condition = "",
              const QDate& lastMaintenance = QDate::currentDate());

    // Геттеры (п.3 ТЗ - доступ к 8 параметрам)
    int id() const;
    QString model() const;
    QString type() const;
    int year() const;
    QString regNumber() const;
    double operatingHours() const;
    QString condition() const;
    QDate lastMaintenance() const;

    // Сеттеры (п.4, п.5 ТЗ - редактирование записей)
    void setId(int id);
    void setModel(const QString& model);
    void setType(const QString& type);
    void setYear(int year);
    void setRegNumber(const QString& regNumber);
    void setOperatingHours(double hours);
    void setCondition(const QString& condition);
    void setLastMaintenance(const QDate& date);

    /**
     * @brief Сериализация в CSV формат
     * @return Строка в CSV формате
     * 
     * Выполнение пунктов ТЗ:
     * - п.2: Запись в текстовый файл
     * - п.4: Защита от поврежденных файлов
     */
    QString toCsv() const;

    /**
     * @brief Десериализация из CSV формата
     * @param line Строка в CSV формате
     * @return Объект Equipment
     * 
     * Выполнение пунктов ТЗ:
     * - п.2: Чтение из текстового файла
     * - п.4: Защита от поврежденных файлов
     */
    static Equipment fromCsv(const QString& line);

private:
    int m_id;                    ///< Уникальный идентификатор (п.3 ТЗ)
    QString m_model;             ///< Модель техники (п.3 ТЗ)
    QString m_type;              ///< Тип техники (п.3 ТЗ)
    int m_year;                  ///< Год выпуска (п.3 ТЗ)
    QString m_regNumber;         ///< Регистрационный номер (п.3 ТЗ)
    double m_operatingHours;     ///< Наработка в часах (п.3 ТЗ)
    QString m_condition;         ///< Техническое состояние (п.3 ТЗ)
    QDate m_lastMaintenance;     ///< Дата последнего ТО (п.3 ТЗ)
};

#endif // EQUIPMENT_H
