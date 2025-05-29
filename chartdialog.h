#ifndef CHARTDIALOG_H
#define CHARTDIALOG_H

#include <QDialog>
#include <QVector>
#include "equipment.h"

namespace Ui {
class ChartDialog;
}

class ChartDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChartDialog(const QVector<Equipment>& equipment, QWidget *parent = nullptr);
    ~ChartDialog();

private slots:
    void on_comboBox_currentIndexChanged(int index);

private:
    Ui::ChartDialog *ui;
    QVector<Equipment> m_equipment;

    void updateChart(int type);
};

#endif // CHARTDIALOG_H
