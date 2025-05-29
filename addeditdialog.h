#ifndef ADDEDITDIALOG_H
#define ADDEDITDIALOG_H

#include <QDialog>
#include "equipment.h"

namespace Ui {
class AddEditDialog;
}

class AddEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddEditDialog(QWidget *parent = nullptr);
    ~AddEditDialog();

    Equipment getEquipment() const;
    void setEquipment(const Equipment &equipment);

private:
    Ui::AddEditDialog *ui;
    Equipment m_equipment;

    void setupValidators();
};

#endif // ADDEDITDIALOG_H
