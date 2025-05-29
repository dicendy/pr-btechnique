#include "addeditdialog.h"
#include "ui_addeditdialog.h"
#include <QRegularExpressionValidator>
#include <QDate>

AddEditDialog::AddEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddEditDialog)
{
    ui->setupUi(this);
    setupValidators();
}

AddEditDialog::~AddEditDialog()
{
    delete ui;
}

Equipment AddEditDialog::getEquipment() const
{
    Equipment eq;
    eq.setModel(ui->modelEdit->text());
    eq.setType(ui->typeComboBox->currentText());
    eq.setYear(ui->yearSpinBox->value());
    eq.setRegNumber(ui->regNumberEdit->text());
    eq.setOperatingHours(ui->hoursSpinBox->value());
    eq.setCondition(ui->conditionComboBox->currentText());
    eq.setLastMaintenance(ui->maintenanceDateEdit->date());
    return eq;
}

void AddEditDialog::setEquipment(const Equipment &equipment)
{
    m_equipment = equipment;
    ui->modelEdit->setText(equipment.model());
    ui->typeComboBox->setCurrentText(equipment.type());
    ui->yearSpinBox->setValue(equipment.year());
    ui->regNumberEdit->setText(equipment.regNumber());
    ui->hoursSpinBox->setValue(equipment.operatingHours());
    ui->conditionComboBox->setCurrentText(equipment.condition());
    ui->maintenanceDateEdit->setDate(equipment.lastMaintenance());
}

void AddEditDialog::setupValidators()
{
    // Рег. номер: 2 буквы, 3 цифры, 2 буквы
    QRegularExpression regExp("[А-ЯA-Z]{2}\\d{3}[А-ЯA-Z]{2}");
    ui->regNumberEdit->setValidator(new QRegularExpressionValidator(regExp, this));

    // Только цифры для года
    ui->yearSpinBox->setRange(1950, QDate::currentDate().year());

    // Часы работы: положительное число
    ui->hoursSpinBox->setMinimum(0);
    ui->hoursSpinBox->setMaximum(99999);
}
