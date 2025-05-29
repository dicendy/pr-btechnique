/**
 * @file equipmentmodel.h
 * @brief Модель данных для управления строительной техникой
 * @author Дищенко Дмитрий Игоревич МГСУ ИЦТМС 2-1
 * 
 * Выполнение пунктов ТЗ:
 * - п.13: Использование Item based объектов
 * - п.14: Использование контейнеров Qt/STL
 * - п.15: Реализация паттерна MVC
 * - п.23: Поддержка Drag-and-drop
 */

#ifndef EQUIPMENTMODEL_H
#define EQUIPMENTMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include "equipment.h"

/**
 * @class EquipmentModel
 * @brief Модель данных для управления строительной техникой
 *
 * Реализует паттерн MVC для работы с данными (п.15 ТЗ)
 * Поддерживает все основные операции CRUD:
 * - Create (добавление записей) - п.2.IV ТЗ
 * - Read (чтение файлов) - п.2.I, п.2.III ТЗ
 * - Update (редактирование записей) - п.2.V ТЗ
 * - Delete (удаление записей) - п.2.VI ТЗ
 * 
 * Дополнительные возможности:
 * - п.18: Сортировка по столбцам
 * - п.23: Drag-and-drop операции
 * - п.4: Защита от поврежденных файлов
 */
class EquipmentModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit EquipmentModel(QObject *parent = nullptr);

    // QAbstractItemModel interface (п.13, п.15 ТЗ)
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // Drag-and-drop support (п.23 ТЗ)
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override;

    /**
     * @brief Загрузка данных из файла
     * @param filename Имя файла для загрузки
     * 
     * Выполнение пунктов ТЗ:
     * - п.2.I: Чтение файла
     * - п.4: Защита от поврежденных файлов
     */
    void loadFromFile(const QString &filename);

    /**
     * @brief Сохранение данных в файл
     * @param filename Имя файла для сохранения
     * 
     * Выполнение пунктов ТЗ:
     * - п.2.II: Запись файла
     */
    void saveToFile(const QString &filename);

    // CRUD operations (п.2 ТЗ)
    /**
     * @brief Добавление нового оборудования
     * @param equipment Объект оборудования для добавления
     * 
     * Выполнение п.2.IV ТЗ: Добавление отдельной записи
     */
    void addEquipment(const Equipment &equipment);

    /**
     * @brief Обновление существующего оборудования
     * @param id Идентификатор оборудования
     * @param equipment Новые данные оборудования
     * 
     * Выполнение п.2.V ТЗ: Редактирование отдельной записи
     */
    void updateEquipment(int id, const Equipment &equipment);

    /**
     * @brief Удаление оборудования
     * @param id Идентификатор оборудования для удаления
     * 
     * Выполнение п.2.VI ТЗ: Удаление отдельной записи
     */
    void removeEquipment(int id);

    /**
     * @brief Получение оборудования по ID
     * @param id Идентификатор оборудования
     * @return Объект Equipment
     */
    Equipment getEquipment(int id) const;

    /**
     * @brief Получение оборудования по индексу
     * @param index Индекс в списке
     * @return Объект Equipment
     */
    Equipment getEquipmentAt(int index) const;

    /**
     * @brief Получение всего списка оборудования
     * @return Вектор всех объектов Equipment
     * 
     * Выполнение п.2.III ТЗ: Вывод содержимого файла
     */
    QVector<Equipment> getAllEquipment() const;

    /**
     * @brief Заголовок файла для проверки целостности
     * 
     * Выполнение п.4 ТЗ: Защита от поврежденных файлов
     */
    static const QString DB_HEADER;

private:
    QVector<Equipment> m_equipment;  ///< Контейнер для хранения данных (п.14 ТЗ)
    int m_nextId = 1;               ///< Счетчик для генерации уникальных ID
};

#endif // EQUIPMENTMODEL_H
