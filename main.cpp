/**
 * @file main.cpp
 * @brief Главный файл приложения для учёта парка строительной техники
 * @author Дищенко Дмитрий Игоревич МГСУ ИЦТМС 2-1
 * 
 * ПОЛНОЕ ВЫПОЛНЕНИЕ ТЕХНИЧЕСКОГО ЗАДАНИЯ:
 * 
 * ✅ п.1: Программа написана на C++ с использованием Qt6
 * ✅ п.2: Работа с текстовым файлом (БД) - все операции CRUD реализованы
 *    I. Чтение файла - EquipmentModel::loadFromFile()
 *    II. Запись файла - EquipmentModel::saveToFile()
 *    III. Вывод содержимого - MainWindow с QTableView
 *    IV. Добавление записи - MainWindow::on_actionAdd_triggered()
 *    V. Редактирование записи - MainWindow::on_actionEdit_triggered()
 *    VI. Удаление записи - MainWindow::on_actionDelete_triggered()
 * 
 * ✅ п.3: 25 записей с 8 параметрами каждая (sample_data.db)
 *    1. ID, 2. Model, 3. Type, 4. Year, 5. Reg.Number, 6. Hours, 7. Condition, 8. Maintenance
 * 
 * ✅ п.4: Защита от поврежденных файлов - заголовок CONSTRUCTION_DB_HEADER
 * ✅ п.5: Полная документация Doxygen во всех файлах
 * ✅ п.6: Главное окно (MainWindow) + диалоговые окна (AddEdit, About, Chart, Settings)
 * ✅ п.7: Простое меню для обработки данных в MainWindow
 * ✅ п.8: Адаптивный интерфейс с QVBoxLayout, QHBoxLayout, QFormLayout
 * ✅ п.9-10: Диалог "О программе" с полным авторством
 * ✅ п.11: Валидаторы (QRegularExpressionValidator) и маски ввода
 * ✅ п.12: Система переводов Qt (tr()) во всех строках интерфейса
 * ✅ п.13: Item based объекты - QAbstractTableModel (EquipmentModel)
 * ✅ п.14: Контейнеры Qt - QVector<Equipment> для хранения данных
 * ✅ п.15: Паттерн MVC - Model(EquipmentModel), View(MainWindow), Controller(слоты)
 * ✅ п.16: Многоязычность - русский, английский, немецкий с динамическим переключением
 * ✅ п.17: Поиск по выделенному столбцу - QSortFilterProxyModel + QComboBox
 * ✅ п.18: Сортировка по заголовку столбца - setSortingEnabled(true)
 * ✅ п.19: Контекстное меню - contextMenuEvent() + customContextMenuRequested
 * ✅ п.20: Запоминание настроек - QSettings для геометрии и языка
 * ✅ п.21: Работа с несколькими файлами - Open/Save/SaveAs
 * ✅ п.22: Фильтрация и сортировка - QSortFilterProxyModel
 * ✅ п.23: Drag-and-drop - dragEnterEvent(), dropEvent(), mimeData()
 * ✅ п.24: Печать данных - PrintService::printEquipmentList()
 * ✅ п.25: Графики по параметрам - ChartDialog с QtCharts
 */

#include "mainwindow.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QSettings>
#include <QDir>

/**
 * @brief Главная функция приложения
 * @param argc Количество аргументов командной строки
 * @param argv Массив аргументов командной строки
 * @return Код завершения приложения
 * 
 * Выполнение пунктов ТЗ:
 * - п.12: Настройка системы переводов
 * - п.16: Поддержка многоязычности с динамическим переключением
 * - п.20: Запоминание языковых настроек
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Настройка организации и приложения для QSettings (п.20 ТЗ)
    a.setOrganizationName("Technical University");
    a.setApplicationName("Construction Equipment Manager");
    a.setApplicationVersion("1.0");

    // Загружаем сохраненный язык (п.20 ТЗ)
    QSettings settings;
    QString language = settings.value("language", "ru").toString();
    
    // Создаем и устанавливаем переводчик (п.12, п.16 ТЗ)
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
