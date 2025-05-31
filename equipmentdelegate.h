#ifndef EQUIPMENTDELEGATE_H
#define EQUIPMENTDELEGATE_H

#include <QStyledItemDelegate>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDateEdit>

/**
 * @class EquipmentDelegate
 * @brief Кастомный делегат для редактирования ячеек таблицы с валидацией
 */
class EquipmentDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit EquipmentDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const override;

private:
    void commitDataAndCloseEditor();
};


#endif // EQUIPMENTDELEGATE_H 
