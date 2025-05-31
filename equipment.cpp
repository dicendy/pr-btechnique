#include "equipment.h"
#include <QStringList>

Equipment::Equipment(int id, const QString& model, const QString& type,
                     int year, const QString& regNumber, double operatingHours,
                     const QString& condition, const QDate& lastMaintenance)
    : m_id(id), m_model(model), m_type(type), m_year(year),
    m_regNumber(regNumber), m_operatingHours(operatingHours),
    m_condition(condition), m_lastMaintenance(lastMaintenance) {}

// Геттеры
int Equipment::id() const { return m_id; }
QString Equipment::model() const { return m_model; }
QString Equipment::type() const { return m_type; }
int Equipment::year() const { return m_year; }
QString Equipment::regNumber() const { return m_regNumber; }
double Equipment::operatingHours() const { return m_operatingHours; }
QString Equipment::condition() const { return m_condition; }
QDate Equipment::lastMaintenance() const { return m_lastMaintenance; }

// Сеттеры
void Equipment::setId(int id) { m_id = id; }
void Equipment::setModel(const QString& model) { m_model = model; }
void Equipment::setType(const QString& type) { m_type = type; }
void Equipment::setYear(int year) { m_year = year; }
void Equipment::setRegNumber(const QString& regNumber) { m_regNumber = regNumber; }
void Equipment::setOperatingHours(double hours) { m_operatingHours = hours; }
void Equipment::setCondition(const QString& condition) { m_condition = condition; }
void Equipment::setLastMaintenance(const QDate& date) { m_lastMaintenance = date; }

QString Equipment::toCsv() const
{
    // Проверяем валидность данных перед сериализацией
    QString model = m_model.isEmpty() ? "Unknown" : m_model;
    QString type = m_type.isEmpty() ? "Unknown" : m_type;
    QString regNumber = m_regNumber.isEmpty() ? "N/A" : m_regNumber;
    QString condition = m_condition.isEmpty() ? "Unknown" : m_condition;

    // Гарантируем, что дата всегда валидна
    QDate maintenance = m_lastMaintenance.isValid() ? m_lastMaintenance : QDate::currentDate();

    return QString("%1|%2|%3|%4|%5|%6|%7|%8")
        .arg(m_id)
        .arg(model)
        .arg(type)
        .arg(m_year)
        .arg(regNumber)
        .arg(m_operatingHours, 0, 'f', 1)
        .arg(condition)
        .arg(maintenance.toString("dd.MM.yyyy"));
}

Equipment Equipment::fromCsv(const QString& line)
{
    if (line.trimmed().isEmpty()) {
        qWarning() << "Empty line provided to fromCsv";
        return Equipment(-1, "", "", 0, "", 0.0, "", QDate());
    }

    QStringList parts = line.split('|');
    if (parts.size() != 8) {
        qWarning() << "Invalid CSV line format. Expected 8 parts, got:" << parts.size();
        qWarning() << "Line:" << line;
        return Equipment(-1, "", "", 0, "", 0.0, "", QDate());
    }

    // Проверяем и парсим каждую часть с обработкой ошибок
    bool idOk = false;
    int id = parts[0].toInt(&idOk);
    if (!idOk) {
        qWarning() << "Invalid ID in CSV:" << parts[0];
        return Equipment(-1, "", "", 0, "", 0.0, "", QDate());
    }

    QString model = parts[1].trimmed();
    QString type = parts[2].trimmed();

    bool yearOk = false;
    int year = parts[3].toInt(&yearOk);
    if (!yearOk || year < 1900 || year > QDate::currentDate().year() + 10) {
        qWarning() << "Invalid year in CSV:" << parts[3];
        year = QDate::currentDate().year();
    }

    QString regNumber = parts[4].trimmed();

    bool hoursOk = false;
    double operatingHours = parts[5].toDouble(&hoursOk);
    if (!hoursOk || operatingHours < 0) {
        qWarning() << "Invalid operating hours in CSV:" << parts[5];
        operatingHours = 0.0;
    }

    QString condition = parts[6].trimmed();

    // Обработка даты
    QDate lastMaintenance;
    if (parts[7].trimmed().isEmpty()) {
        lastMaintenance = QDate::currentDate();
        qWarning() << "Empty date in CSV, using current date";
    } else {
        lastMaintenance = QDate::fromString(parts[7].trimmed(), "dd.MM.yyyy");
        if (!lastMaintenance.isValid()) {
            qWarning() << "Invalid date in CSV:" << parts[7];
            lastMaintenance = QDate::currentDate();
        }
    }

    return Equipment(id, model, type, year, regNumber, operatingHours, condition, lastMaintenance);
}
