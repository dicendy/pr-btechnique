#include "chartdialog.h"
#include "ui_chartdialog.h"
#include <QChartView>
#include <QBarSeries>
#include <QBarSet>
#include <QLegend>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPieSeries>
#include <QChart>
#include <QMap>


ChartDialog::ChartDialog(const QVector<Equipment>& equipment, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChartDialog),
    m_equipment(equipment)
{
    ui->setupUi(this);

    ui->comboBox->addItem(tr("Equipment by Year"));
    ui->comboBox->addItem(tr("Operating Hours"));
    ui->comboBox->addItem(tr("Condition Statistics"));

    updateChart(0);
}

ChartDialog::~ChartDialog()
{
    delete ui;
}

void ChartDialog::on_comboBox_currentIndexChanged(int index)
{
    updateChart(index);
}

void ChartDialog::updateChart(int type)
{
    QChart *chart = new QChart();

    if (type == 0) { // Equipment by Year
        QMap<int, int> yearCount;
        for (const Equipment& eq : m_equipment) {
            yearCount[eq.year()]++;
        }

        QBarSeries *series = new QBarSeries();
        QBarSet *set = new QBarSet(tr("Equipment Count"));

        QStringList categories;
        for (auto it = yearCount.begin(); it != yearCount.end(); ++it) {
            *set << it.value();
            categories << QString::number(it.key());
        }

        series->append(set);
        chart->addSeries(series);

        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);

        chart->setTitle(tr("Equipment Count by Year"));
    }
    else if (type == 1) { // Operating Hours
        QBarSeries *series = new QBarSeries();
        QBarSet *set = new QBarSet(tr("Operating Hours"));

        QStringList categories;
        for (const Equipment& eq : m_equipment) {
            *set << eq.operatingHours();
            categories << eq.model();
        }

        series->append(set);
        chart->addSeries(series);

        QBarCategoryAxis *axisX = new QBarCategoryAxis();
        axisX->append(categories);
        chart->addAxis(axisX, Qt::AlignBottom);
        series->attachAxis(axisX);

        QValueAxis *axisY = new QValueAxis();
        chart->addAxis(axisY, Qt::AlignLeft);
        series->attachAxis(axisY);

        chart->setTitle(tr("Operating Hours by Equipment"));
    }
    else { // Condition Statistics
        QMap<QString, int> conditionCount;
        for (const Equipment& eq : m_equipment) {
            conditionCount[eq.condition()]++;
        }

        QPieSeries *series = new QPieSeries();
        for (auto it = conditionCount.begin(); it != conditionCount.end(); ++it) {
            series->append(it.key(), it.value());
        }

        chart->addSeries(series);
        chart->setTitle(tr("Equipment Condition Statistics"));
    }

    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);
}
