#include "equipmentdelegate.h"
#include <QRegularExpressionValidator>
#include <QDate>

EquipmentDelegate::EquipmentDelegate(QObject *parent) 
    : QStyledItemDelegate(parent)
{
}

QWidget *EquipmentDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    Q_UNUSED(option)
    
    int column = index.column();
    
    switch (column) {
    case 0: // ID - не редактируется
        return nullptr;
        
    case 1: // Model - обычный текст
        {
            QLineEdit *editor = new QLineEdit(parent);
            return editor;
        }
        
    case 2: // Type - выпадающий список
        {
            QComboBox *combo = new QComboBox(parent);
            combo->addItems({tr("Excavator"), tr("Bulldozer"), tr("Crane"), 
                           tr("Loader"), tr("Grader"), tr("Other")});
            combo->setEditable(false);
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &EquipmentDelegate::commitDataAndCloseEditor);
            return combo;
        }
        
    case 3: // Year - спинбокс для года
        {
            QSpinBox *spinBox = new QSpinBox(parent);
            spinBox->setRange(1900, 2100);
            spinBox->setValue(QDate::currentDate().year());
            return spinBox;
        }
        
    case 4: // Reg. Number - текст с валидатором
        {
            QLineEdit *editor = new QLineEdit(parent);
            QRegularExpression regExp("[А-ЯA-Z]{1}\\d{3}[А-ЯA-Z]{2}");
            editor->setPlaceholderText("А111ВВ");
            return editor;
        }
        
    case 5: // Operating Hours - дробные числа
        {
            QLineEdit *editor = new QLineEdit(parent);
            QRegularExpressionValidator *validator = 
                new QRegularExpressionValidator(QRegularExpression("^\\d+(\\.\\d+)?$"), editor);
            editor->setValidator(validator);
            editor->setPlaceholderText("123.4");
            return editor;
        }
        
    case 6: // Condition - выпадающий список
        {
            QComboBox *combo = new QComboBox(parent);
            combo->addItems({tr("Excellent"), tr("Good"), tr("Satisfactory"), 
                           tr("Poor"), tr("Critical")});
            combo->setEditable(false);
            return combo;
            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &EquipmentDelegate::commitDataAndCloseEditor);
        }
        
    case 7: // Last Maintenance - дата
        {
            QDateEdit *dateEdit = new QDateEdit(parent);
            dateEdit->setDate(QDate::currentDate());
            dateEdit->setDisplayFormat("dd.MM.yyyy");
            dateEdit->setCalendarPopup(true);
            return dateEdit;
        }
        
    default:
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}

void EquipmentDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    int column = index.column();
    QString value = index.model()->data(index, Qt::EditRole).toString();
    
    switch (column) {
    case 1: // Model
    case 4: // Reg. Number
    case 5: // Operating Hours
        {
            QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
            if (lineEdit) {
                lineEdit->setText(value);
            }
        }
        break;
        
    case 2: // Type
        {
            QComboBox *combo = qobject_cast<QComboBox*>(editor);
            if (combo) {
                // Сопоставляем исходные значения с переводами
                int idx = -1;
                if (value == "Excavator" || value == tr("Excavator")) idx = 0;
                else if (value == "Bulldozer" || value == tr("Bulldozer")) idx = 1;
                else if (value == "Crane" || value == tr("Crane")) idx = 2;
                else if (value == "Loader" || value == tr("Loader")) idx = 3;
                else if (value == "Grader" || value == tr("Grader")) idx = 4;
                else if (value == "Other" || value == tr("Other")) idx = 5;
                
                if (idx >= 0) combo->setCurrentIndex(idx);
            }
        }
        break;
        
    case 6: // Condition
        {
            QComboBox *combo = qobject_cast<QComboBox*>(editor);
            if (combo) {
                // Сопоставляем исходные значения с переводами
                int idx = -1;
                if (value == "Excellent" || value == tr("Excellent")) idx = 0;
                else if (value == "Good" || value == tr("Good")) idx = 1;
                else if (value == "Satisfactory" || value == tr("Satisfactory")) idx = 2;
                else if (value == "Poor" || value == tr("Poor")) idx = 3;
                else if (value == "Critical" || value == tr("Critical")) idx = 4;
                
                if (idx >= 0) combo->setCurrentIndex(idx);
            }
        }
        break;
        
    case 3: // Year
        {
            QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor);
            if (spinBox) {
                spinBox->setValue(value.toInt());
            }
        }
        break;
        
    case 7: // Last Maintenance
        {
            QDateEdit *dateEdit = qobject_cast<QDateEdit*>(editor);
            if (dateEdit) {
                QDate date = QDate::fromString(value, "dd.MM.yyyy");
                if (date.isValid()) {
                    dateEdit->setDate(date);
                }
            }
        }
        break;
        
    default:
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void EquipmentDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    int column = index.column();
    
    switch (column) {
    case 1: // Model
    case 4: // Reg. Number
        {
            QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
            if (lineEdit) {
                model->setData(index, lineEdit->text(), Qt::EditRole);
            }
        }
        break;

    case 5: // Operating Hours
        {
            QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
            if (lineEdit) {
                model->setData(index, lineEdit->text(), Qt::EditRole);
            }
        }
        break;
        
    case 2: // Type
    case 6: // Condition
        {
            QComboBox *combo = qobject_cast<QComboBox*>(editor);
            if (combo) {
                model->setData(index, combo->currentText(), Qt::EditRole);
            }
        }
        break;
        
    case 3: // Year
        {
            QSpinBox *spinBox = qobject_cast<QSpinBox*>(editor);
            if (spinBox) {
                model->setData(index, spinBox->value(), Qt::EditRole);
            }
        }
        break;
        
    case 7: // Last Maintenance
        {
            QDateEdit *dateEdit = qobject_cast<QDateEdit*>(editor);
            if (dateEdit) {
                model->setData(index, dateEdit->date().toString("dd.MM.yyyy"), Qt::EditRole);
            }
        }
        break;
        
    default:
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void EquipmentDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    Q_UNUSED(index)
    editor->setGeometry(option.rect);
}

void EquipmentDelegate::commitDataAndCloseEditor()
{
    QWidget *editor = qobject_cast<QWidget *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}
