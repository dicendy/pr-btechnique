#include "equipmentmodel.h"
#include <QFile>
#include <QTextStream>
#include <QMimeData>
#include <QUrl>
#include <QDebug>

const QString EquipmentModel::DB_HEADER = "CONSTRUCTION_DB_HEADER";

EquipmentModel::EquipmentModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

int EquipmentModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_equipment.size();
}

int EquipmentModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return 8; // 8 параметров оборудования
}

QVariant EquipmentModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_equipment.size() || index.row() < 0) {
        return QVariant();
    }

    const Equipment &item = m_equipment[index.row()];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
        case 0: return item.id();
        case 1: return item.model();
        case 2: return item.type();
        case 3: return item.year();
        case 4: return item.regNumber();
        case 5: return QString::number(item.operatingHours(), 'f', 1);
        case 6: return item.condition();
        case 7: return item.lastMaintenance().toString("dd.MM.yyyy");
        default: return QVariant();
        }
    }

    return QVariant();
}

QVariant EquipmentModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) return QVariant();

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return tr("ID");
        case 1: return tr("Model");
        case 2: return tr("Type");
        case 3: return tr("Year");
        case 4: return tr("Reg. Number");
        case 5: return tr("Operating Hours");
        case 6: return tr("Condition");
        case 7: return tr("Last Maintenance");
        }
    }
    return QVariant();
}

bool EquipmentModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != Qt::EditRole || index.row() >= m_equipment.size() || index.row() < 0) {
        return false;
    }

    Equipment &item = m_equipment[index.row()];
    bool changed = false;

    switch (index.column()) {
    case 1:
        if (value.toString() != item.model()) {
            item.setModel(value.toString());
            changed = true;
        }
        break;
    case 2:
        if (value.toString() != item.type()) {
            item.setType(value.toString());
            changed = true;
        }
        break;
    case 3:
        if (value.toInt() != item.year()) {
            item.setYear(value.toInt());
            changed = true;
        }
        break;
    case 4:
        if (value.toString() != item.regNumber()) {
            item.setRegNumber(value.toString());
            changed = true;
        }
        break;
    case 5:
        if (qAbs(value.toDouble() - item.operatingHours()) > 0.01) {
            item.setOperatingHours(value.toDouble());
            changed = true;
        }
        break;
    case 6:
        if (value.toString() != item.condition()) {
            item.setCondition(value.toString());
            changed = true;
        }
        break;
    case 7:
        if (value.toDate() != item.lastMaintenance()) {
            item.setLastMaintenance(value.toDate());
            changed = true;
        }
        break;
    default:
        return false;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        return true;
    }
    return false;
}

Qt::ItemFlags EquipmentModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractTableModel::flags(index);

    if (index.isValid()) {
        return defaultFlags | Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    }
    return defaultFlags | Qt::ItemIsDropEnabled;
}

QStringList EquipmentModel::mimeTypes() const
{
    return {"text/plain", "text/csv"};
}

QMimeData *EquipmentModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mimeData = new QMimeData();
    QStringList lines;

    for (const QModelIndex &index : indexes) {
        if (index.isValid() && index.column() == 0) {
            Equipment eq = m_equipment[index.row()];
            lines << eq.toCsv();
        }
    }

    mimeData->setText(lines.join("\n"));
    return mimeData;
}

bool EquipmentModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                  int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction) return true;

    if (!data->hasText()) return false;

    QStringList lines = data->text().split('\n');
    for (const QString &line : lines) {
        if (line.trimmed().isEmpty()) continue;
        Equipment eq = Equipment::fromCsv(line);
        if (eq.id() >= 0) {
            addEquipment(eq);
        }
    }
    return true;
}

Qt::DropActions EquipmentModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

void EquipmentModel::loadFromFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw std::runtime_error(tr("Cannot open file").toStdString());
    }

    QTextStream in(&file);

    // Проверка заголовка (п.4 ТЗ)
    QString header = in.readLine();
    if (header != DB_HEADER) {
        throw std::runtime_error(tr("Invalid file format").toStdString());
    }

    beginResetModel();
    m_equipment.clear();

    while (!in.atEnd()) {
        QString line = in.readLine();
        Equipment eq = Equipment::fromCsv(line);
        if (eq.id() >= 0) {
            m_equipment.append(eq);
            if (eq.id() >= m_nextId) m_nextId = eq.id() + 1;
        }
    }

    endResetModel();
    file.close();
}

void EquipmentModel::saveToFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        throw std::runtime_error(tr("Cannot save file").toStdString());
    }

    QTextStream out(&file);
    out << DB_HEADER << "\n";

    for (const Equipment &eq : m_equipment) {
        out << eq.toCsv() << "\n";
    }

    file.close();
}

void EquipmentModel::addEquipment(const Equipment &equipment)
{
    beginInsertRows(QModelIndex(), m_equipment.size(), m_equipment.size());
    Equipment newEq = equipment;
    newEq.setId(m_nextId++);
    m_equipment.append(newEq);
    endInsertRows();
}

void EquipmentModel::updateEquipment(int id, const Equipment &equipment)
{
    for (int i = 0; i < m_equipment.size(); ++i) {
        if (m_equipment[i].id() == id) {
            // Создаем копию и сохраняем оригинальный ID
            Equipment updated = equipment;
            updated.setId(id);

            // Обновляем данные
            m_equipment[i] = updated;

            // Уведомляем представление об изменениях
            QModelIndex topLeft = index(i, 0);
            QModelIndex bottomRight = index(i, columnCount() - 1);
            emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole, Qt::EditRole});

            qDebug() << "Equipment updated successfully, ID:" << id;
            return;
        }
    }

    // Если оборудование не найдено
    qWarning() << "Equipment with ID" << id << "not found for update";
    throw std::runtime_error("Equipment not found for update");
}

void EquipmentModel::removeEquipment(int id)
{
    for (int i = 0; i < m_equipment.size(); ++i) {
        if (m_equipment[i].id() == id) {
            // Удаление с уведомлением модели
            beginRemoveRows(QModelIndex(), i, i);
            m_equipment.removeAt(i);
            endRemoveRows();

            qDebug() << "Equipment removed successfully, ID:" << id;
            return;
        }
    }

    // Если оборудование не найдено
    qWarning() << "Equipment with ID" << id << "not found for removal";
    throw std::runtime_error("Equipment not found for removal");
}

Equipment EquipmentModel::getEquipment(int id) const
{
    for (const Equipment &eq : m_equipment) {
        if (eq.id() == id) {
            return eq;
        }
    }

    // Возвращаем пустой объект с ID = -1 для индикации ошибки
    qWarning() << "Equipment with ID" << id << "not found";
    return Equipment(-1, "", "", 0, "", 0.0, "", QDate());
}

Equipment EquipmentModel::getEquipmentAt(int index) const
{
    if (index >= 0 && index < m_equipment.size()) {
        return m_equipment[index];
    }

    // Возвращаем пустой объект с ID = -1 для индикации ошибки
    qWarning() << "Equipment index" << index << "is out of range (size:" << m_equipment.size() << ")";
    return Equipment(-1, "", "", 0, "", 0.0, "", QDate());
}

QVector<Equipment> EquipmentModel::getAllEquipment() const
{
    return m_equipment;
}
