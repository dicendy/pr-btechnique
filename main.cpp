/**
 * @file main.cpp
 * @brief ������� ���� ���������� ��� ����� ����� ������������ �������
 * @author ������� ������� �������� ���� ����� 2-1
 * 
 * ������ ���������� ������������ �������:
 * 
 * �.1: ��������� �������� �� C++ � �������������� Qt6
 * �.2: ������ � ��������� ������ (��) - ��� �������� CRUD �����������
 *    I. ������ ����� - EquipmentModel::loadFromFile()
 *    II. ������ ����� - EquipmentModel::saveToFile()
 *    III. ����� ����������� - MainWindow � QTableView
 *    IV. ���������� ������ - MainWindow::on_actionAdd_triggered()
 *    V. �������������� ������ - MainWindow::on_actionEdit_triggered()
 *    VI. �������� ������ - MainWindow::on_actionDelete_triggered()
 * 
 * �.3: 25 ������� � 8 ����������� ������ (sample_data.db)
 *    1. ID, 2. Model, 3. Type, 4. Year, 5. Reg.Number, 6. Hours, 7. Condition, 8. Maintenance
 * 
 * �.4: ������ �� ������������ ������ - ��������� CONSTRUCTION_DB_HEADER
 * �.5: ������ ������������ Doxygen �� ���� ������
 * �.6: ������� ���� (MainWindow) + ���������� ���� (AddEdit, About, Chart, Settings)
 * �.7: ������� ���� ��� ��������� ������ � MainWindow
 * �.8: ���������� ��������� � QVBoxLayout, QHBoxLayout, QFormLayout
 * �.9-10: ������ "� ���������" � ������ ����������
 * �.11: ���������� (QRegularExpressionValidator) � ����� �����
 * �.12: ������� ��������� Qt (tr()) �� ���� ������� ����������
 * �.13: Item based ������� - QAbstractTableModel (EquipmentModel)
 * �.14: ���������� Qt - QVector<Equipment> ��� �������� ������
 * �.15: ������� MVC - Model(EquipmentModel), View(MainWindow), Controller(�����)
 * �.16: �������������� - �������, ����������, �������� � ������������ �������������
 * �.17: ����� �� ����������� ������� - QSortFilterProxyModel + QComboBox
 * �.18: ���������� �� ��������� ������� - setSortingEnabled(true)
 * �.19: ����������� ���� - contextMenuEvent() + customContextMenuRequested
 * �.20: ����������� �������� - QSettings ��� ��������� � �����
 * �.21: ������ � ����������� ������� - Open/Save/SaveAs
 * �.22: ���������� � ���������� - QSortFilterProxyModel
 * �.23: Drag-and-drop - dragEnterEvent(), dropEvent(), mimeData()
 * �.24: ������ ������ - PrintService::printEquipmentList()
 * �.25: ������� �� ���������� - ChartDialog � QtCharts
 */

#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSettings>
#include <QDir>

/**
 * @brief ������� ������� ����������
 * @param argc ���������� ���������� ��������� ������
 * @param argv ������ ���������� ��������� ������
 * @return ��� ���������� ����������
 * 
 * ���������� ������� ��:
 * - �.12: ��������� ������� ���������
 * - �.16: ��������� �������������� � ������������ �������������
 * - �.20: ����������� �������� ��������
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // ��������� ����������� � ���������� ��� QSettings (�.20 ��)
    a.setOrganizationName("Technical University");
    a.setApplicationName("Construction Equipment Manager");
    a.setApplicationVersion("1.0");

    // ��������� ����������� ���� (�.20 ��)
    QSettings settings;
    QString language = settings.value("language", "ru").toString();
    
    // ������� � ������������� ���������� (�.12, �.16 ��)
    QTranslator translator;
    
    if (language == "ru") {
        bool loaded = translator.load("translations/app_ru.qm");
        if (loaded) {
            a.installTranslator(&translator);
        } else {
            qDebug() << "main.cpp: Failed to load Russian translation";
        }
    } else if (language == "de") {
        bool loaded = translator.load("translations/app_de.qm");
        if (loaded) {
            a.installTranslator(&translator);
        } else {
            qDebug() << "main.cpp: Failed to load German translation";
        }
    } else if (language == "en") {
        bool loaded = translator.load("translations/app_en.qm");
        if (loaded) {
            a.installTranslator(&translator);
        } else {
            qDebug() << "main.cpp: Failed to load English translation";
        }
    }
    
    if (!translator.isEmpty()) {
        a.installTranslator(&translator);
    }

    MainWindow w;
    w.show();
    return a.exec();
}
