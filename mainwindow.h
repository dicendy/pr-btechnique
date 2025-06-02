/**
 * @file mainwindow.h
 * @brief Главное окно приложения для учёта парка строительной техники
 * @author Дищенко Дмитрий Игоревич МГСУ ИЦТМС 2-1
 * 
 * Выполнение пунктов ТЗ:
 * - п.6: Главное окно и диалоговые окна
 * - п.7: Простое меню для обработки данных
 * - п.8: Элементы интерфейса не расползаются при изменении размеров
 * - п.16: Динамическое переключение языков
 * - п.17: Поиск по выделенному столбцу
 * - п.18: Сортировка по заголовку столбца
 * - п.19: Контекстное меню
 * - п.20: Запоминание настроек интерфейса
 * - п.23: Drag-and-drop
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTranslator>
#include <QSortFilterProxyModel>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QTimer>
#include "equipmentmodel.h"
#include "settings.h"
#include "equipmentdelegate.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MultiSelectFilterProxyModel
 * @brief Кастомный прокси-фильтр для поддержки множественного выбора значений
 */
class MultiSelectFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit MultiSelectFilterProxyModel(QObject *parent = nullptr);
    
    void setColumnFilters(int column, const QStringList &filters);
    void clearColumnFilters(int column);
    QStringList getColumnFilters(int column) const;
    
protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QMap<int, QStringList> m_columnFilters;
};

/**
 * @class MainWindow
 * @brief Главное окно приложения
 * 
 * Реализует основной интерфейс пользователя с поддержкой:
 * - п.7: Меню для основных операций
 * - п.15: Паттерн MVC (View компонент)
 * - п.16: Многоязычность
 * - п.17,22: Поиск и фильтрация
 * - п.18: Сортировка
 * - п.19: Контекстное меню
 * - п.20: Сохранение настроек
 * - п.23: Drag-and-drop
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // События интерфейса (п.19, п.23 ТЗ)
    void contextMenuEvent(QContextMenuEvent *event) override;  ///< Контекстное меню (п.19 ТЗ)
    void closeEvent(QCloseEvent *event) override;              ///< Сохранение настроек при закрытии (п.20 ТЗ)
    void dragEnterEvent(QDragEnterEvent *event) override;      ///< Drag-and-drop поддержка (п.23 ТЗ)
    void dropEvent(QDropEvent *event) override;                ///< Drag-and-drop поддержка (п.23 ТЗ)

private slots:
    // Файловые операции (п.2 ТЗ)
    void on_actionNew_triggered();     ///< Создание нового файла
    void on_actionOpen_triggered();    ///< Открытие файла (п.2.I ТЗ)
    void on_actionSave_triggered();    ///< Сохранение файла (п.2.II ТЗ)
    void on_actionSaveAs_triggered();  ///< Сохранение файла как... (п.2.II ТЗ)
    void on_actionPrint_triggered();   ///< Печать данных (п.24 ТЗ)
    void on_actionExit_triggered();    ///< Выход из приложения

    // Операции с данными (п.2 ТЗ)
    void on_actionAdd_triggered();     ///< Добавление записи (п.2.IV ТЗ)
    void on_actionEdit_triggered();    ///< Редактирование записи (п.2.V ТЗ)
    void on_actionDelete_triggered();  ///< Удаление записи (п.2.VI ТЗ)
    void on_actionFind_triggered();    ///< Поиск данных (п.17 ТЗ)

    // Настройки и справка (п.9, п.10 ТЗ)
    void on_actionSettings_triggered(); ///< Настройки приложения
    void on_actionAbout_triggered();    ///< О программе (п.9, п.10 ТЗ)

    // Языки (п.16 ТЗ)
    void switchToEnglish();  ///< Переключение на английский (п.16 ТЗ)
    void switchToRussian();  ///< Переключение на русский (п.16 ТЗ)
    void switchToGerman();   ///< Переключение на немецкий (п.16 ТЗ)

    // Графики (п.25 ТЗ)
    void on_actionShowChart_triggered(); ///< Отображение графиков (п.25 ТЗ)

    // Новые слоты для дополнительной функциональности
    void onTableSelectionChanged();     ///< Обработка изменения выделения в таблице
    void showColumnFilterMenu(const QPoint& pos); ///< Показ меню фильтрации столбца
    void onHeaderClicked(int logicalIndex); ///< Обработка нажатия на заголовок столбца
    void performDelayedSearch();        ///< Выполнение поиска с задержкой

public slots:
    /// Открытие нового окна с документом
    void openNewWindow(const QString &filename = QString());

private:
    Ui::MainWindow *ui;                      ///< Интерфейс пользователя
    static QTranslator *s_translator;        ///< Глобальный переводчик для динамического переключения (п.16 ТЗ)
    EquipmentModel *m_model;                 ///< Модель данных (п.15 ТЗ)
    MultiSelectFilterProxyModel *m_proxyModel; ///< Прокси-модель для фильтрации и сортировки (п.17,18,22 ТЗ)
    QString m_currentFile;                   ///< Текущий открытый файл
    Settings *m_settings;                    ///< Диалог настроек
    QMenu *m_languageMenu;
    QMap<int, QStringList> m_columnFilters;  ///< Активные фильтры по столбцам
    QTimer *m_searchTimer;                   ///< Таймер для дебаунсинга поиска
    bool m_searchInProgress;                 ///< Флаг для предотвращения рекурсивных вызовов поиска

    bool isModelValid() const;

    // Методы настройки интерфейса
    void setupTable();           ///< Настройка таблицы (п.8,18 ТЗ)
    void setupMenu();            ///< Настройка меню (п.7,19 ТЗ)
    void loadSettings();         ///< Загрузка настроек (п.20 ТЗ)
    void saveSettings();         ///< Сохранение настроек (п.20 ТЗ)
    void refreshView();          ///< Обновление представления
    void updateWindowTitle();    ///< Обновление заголовка окна
    void createLanguageMenu();   ///< Создание меню языков (п.16 ТЗ)
    void retranslateUi();        ///< Перевод интерфейса (п.16 ТЗ)
    bool maybeSave();            ///< Проверка необходимости сохранения

    // Drag-and-drop (п.23 ТЗ)
    void setupDragAndDrop();     ///< Настройка drag-and-drop (п.23 ТЗ)
    
    // Валидация данных
    bool validateCellData(int column, const QVariant &value, const QModelIndex &index) const;
    void setupColumnFilters();   ///< Настройка фильтров для столбцов
    void showFilterDialog(int column, const QPoint &pos); ///< Показ диалога фильтрации для столбца
    
    // Управление окнами
    static QList<MainWindow*> s_openWindows; ///< Список открытых окон
    void registerWindow();      ///< Регистрация окна в списке
    void unregisterWindow();    ///< Удаление окна из списка
};
#endif // MAINWINDOW_H
