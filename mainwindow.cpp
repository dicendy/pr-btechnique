#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addeditdialog.h"
#include "aboutdialog.h"
#include "settings.h"
#include "printservice.h"
#include "chartdialog.h"
#include "equipmentdelegate.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QMenu>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QApplication>
#include <QValidator>
#include <QRegularExpressionValidator>
#include <QTimer>
#include <QLibraryInfo>
#include <QTranslator>

MultiSelectFilterProxyModel::MultiSelectFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void MultiSelectFilterProxyModel::setColumnFilters(int column, const QStringList &filters)
{
    m_columnFilters[column] = filters;
    invalidateFilter();
}

void MultiSelectFilterProxyModel::clearColumnFilters(int column)
{
    m_columnFilters.remove(column);
    invalidateFilter();
}

QStringList MultiSelectFilterProxyModel::getColumnFilters(int column) const
{
    return m_columnFilters.value(column);
}

bool MultiSelectFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    // --- УСИЛЕННАЯ ПРОВЕРКА ИСХОДНОЙ МОДЕЛИ ---
    QAbstractItemModel* currentSourceModel = sourceModel();
    if (!currentSourceModel) {
        qDebug() << "MultiSelectFilterProxyModel::filterAcceptsRow: sourceModel is null, row:" << sourceRow;
        return false;
    }

    // Проверяем валидность индекса строки в контексте текущей исходной модели
    if (sourceRow < 0 || sourceRow >= currentSourceModel->rowCount(sourceParent)) {
        qDebug() << "MultiSelectFilterProxyModel::filterAcceptsRow: Invalid row index:" << sourceRow << "for rowCount:" << currentSourceModel->rowCount(sourceParent);
        return false;
    }

    // --- КРИТИЧЕСКОЕ ИСПРАВЛЕНИЕ: НЕ ВЫЗЫВАЕМ БАЗОВЫЙ КЛАСС В ОПРЕДЕЛЕННЫХ СЛУЧАЯХ ---
    // Проверяем, есть ли у нас активные column filters
    bool hasColumnFilters = !m_columnFilters.isEmpty();
    bool baseAccepts = true;
    
    // Если нет column filters, то проверяем базовый фильтр
    if (!hasColumnFilters) {
        try {
            baseAccepts = QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
        } catch (...) {
            qWarning() << "Exception in QSortFilterProxyModel::filterAcceptsRow for row:" << sourceRow;
            return false;
        }
        
        if (!baseAccepts) {
            return false;
        }
    }

    // Проверяем столбцовые фильтры только если они есть
    if (hasColumnFilters) {
        for (auto it = m_columnFilters.constBegin(); it != m_columnFilters.constEnd(); ++it) {
            int column = it.key();
            const QStringList &allowedValues = it.value();

            // Проверяем валидность номера столбца
            if (column < 0 || column >= currentSourceModel->columnCount(sourceParent)) {
                qWarning() << "MultiSelectFilterProxyModel::filterAcceptsRow: Invalid column:" << column << "for columnCount:" << currentSourceModel->columnCount(sourceParent);
                continue;
            }

            // Если список фильтров пустой, это означает "не показывать ничего"
            if (allowedValues.isEmpty()) {
                return false;
            }

            QModelIndex index = currentSourceModel->index(sourceRow, column, sourceParent);
            if (!index.isValid()) {
                qWarning() << "MultiSelectFilterProxyModel::filterAcceptsRow: Invalid index for row:" << sourceRow << "column:" << column;
                continue;
            }

            QString value = currentSourceModel->data(index).toString();

            if (!allowedValues.contains(value)) {
                return false;
            }
        }
    }
    
    // Если есть column filters, проверяем базовый фильтр только в конце
    if (hasColumnFilters) {
        try {
            baseAccepts = QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
        } catch (...) {
            qWarning() << "Exception in QSortFilterProxyModel::filterAcceptsRow for row:" << sourceRow;
            return false;
        }
    }
    
    return baseAccepts;
}

// Определение статического члена (п.16 ТЗ)
QTranslator *MainWindow::s_translator = nullptr;
QTranslator *MainWindow::s_translator2 = nullptr;
QList<MainWindow*> MainWindow::s_openWindows;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new EquipmentModel(this))
    , m_proxyModel(new MultiSelectFilterProxyModel(this))
    , m_settings(new Settings(this))
    , m_searchTimer(new QTimer(this))
    , m_searchInProgress(false)  // Добавляем флаг для предотвращения рекурсии
{
    ui->setupUi(this);

    // Регистрация окна
    registerWindow();

    // Настройка модели и прокси
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(-1); // Поиск по всем столбцам
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    ui->tableView->setModel(m_proxyModel);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);
    
    // Установка кастомного делегата для валидации
    ui->tableView->setItemDelegate(new EquipmentDelegate(this));

    // Настройка интерфейса
    setupTable();
    setupMenu();
    createLanguageMenu();
    loadSettings();
    setupDragAndDrop();
    setupColumnFilters();

    // Настройка таймера для дебаунсинга поиска
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(300); // 300 мс задержка
    connect(m_searchTimer, &QTimer::timeout, this, &MainWindow::performDelayedSearch);

    // Подключение сигналов для поиска в реальном времени (п.17, п.22 ТЗ)
    connect(ui->searchEdit, &QLineEdit::textChanged, this, [this]() {
        // Перезапускаем таймер при каждом изменении текста
        if (!m_searchInProgress) {
            m_searchTimer->stop();
            m_searchTimer->start();
        }
    });
    
    // Подключение сигнала изменения выделения - убираем автоматический поиск
    if (ui->tableView->selectionModel()) {
        connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &MainWindow::onTableSelectionChanged);
    }

    // Статус бар
    ui->statusbar->showMessage(tr("Ready"));
    
    // Блокируем сигналы во время начальной настройки
    ui->searchEdit->blockSignals(true);
    ui->searchEdit->blockSignals(false);
    
    // Загружаем язык из настроек при инициализации (п.16, п.20 ТЗ)
    QString savedLanguage = QSettings().value("language", "ru").toString();
    if (savedLanguage == "en") {
        switchToEnglish();
    } else if (savedLanguage == "de") {
        switchToGerman();
    } else {
        switchToRussian();
    }
}

MainWindow::~MainWindow()
{
    unregisterWindow();
    saveSettings();
    delete ui;
}

void MainWindow::registerWindow()
{
    if (!s_openWindows.contains(this)) {
        s_openWindows.append(this);
    }
}

void MainWindow::unregisterWindow()
{
    s_openWindows.removeAll(this);
}

void MainWindow::setupColumnFilters()
{
    // Настройка контекстного меню для заголовков столбцов
    ui->tableView->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView->horizontalHeader(), &QHeaderView::customContextMenuRequested,
            this, &MainWindow::showColumnFilterMenu);
}

void MainWindow::showColumnFilterMenu(const QPoint& pos)
{
    int column = ui->tableView->horizontalHeader()->logicalIndexAt(pos);
    if (column >= 0) {
        showFilterDialog(column, pos);
    }
}

void MainWindow::showFilterDialog(int column, const QPoint &pos)
{
    // Проверяем валидность моделей и предотвращаем вызов во время поиска
    if (!isModelValid() || m_searchInProgress) {
        return;
    }
    
    // Проверяем валидность номера столбца
    if (column < 0 || column >= m_model->columnCount()) {
        qWarning() << "Invalid column for filter dialog:" << column;
        return;
    }
    
    // Получаем уникальные значения для столбца
    QSet<QString> uniqueValues;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QModelIndex index = m_model->index(row, column);
        QString value = index.data().toString();
        if (!value.isEmpty()) {
            uniqueValues.insert(value);
        }
    }

    // Добавляем опцию для пустых значений
    bool hasEmptyValues = false;
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QModelIndex index = m_model->index(row, column);
        if (index.data().toString().isEmpty()) {
            hasEmptyValues = true;
            break;
        }
    }

    QStringList values = uniqueValues.values();
    values.sort();

    QMenu menu;
    QAction *selectAll = menu.addAction(tr("Select All"));
    QAction *deselectAll = menu.addAction(tr("Deselect All"));
    menu.addSeparator();

    // Получаем текущие фильтры
    QStringList currentFilters = m_proxyModel->getColumnFilters(column);
    bool allSelected = currentFilters.isEmpty();

    // Делаем кнопки управления некликабельными для menu.exec()
    selectAll->setData("select_all_button");
    deselectAll->setData("deselect_all_button");

    QList<QAction*> valueActions;

    // Добавляем опцию для пустых значений
    if (hasEmptyValues) {
        QAction *emptyAction = menu.addAction(tr("(Empty)"));
        emptyAction->setCheckable(true);
        emptyAction->setChecked(allSelected || currentFilters.contains(""));
        emptyAction->setData("");
        valueActions.append(emptyAction);
    }

    // Добавляем все уникальные значения
    for (const QString &value : values) {
        QAction *action = menu.addAction(value);
        action->setCheckable(true);
        action->setChecked(allSelected || currentFilters.contains(value));
        action->setData(value);
        valueActions.append(action);
    }

    // Обработка кнопки "Выделить все" - подключаем к triggered
    connect(selectAll, &QAction::triggered, [&valueActions, &menu, this, column]() {
        for (QAction *action : valueActions) {
            action->setChecked(true);
        }
        // Применяем фильтр - показываем все элементы (очищаем фильтр для этого столбца)
        m_proxyModel->clearColumnFilters(column);

        menu.update();
        menu.repaint();
    });

    // Обработка кнопки "Снять выделение" - подключаем к triggered, но не блокируем меню
    connect(deselectAll, &QAction::triggered, [&valueActions, &menu, this, column]() {
        for (QAction *action : valueActions) {
            action->setChecked(false);
        }
        // Применяем пустой фильтр - скрываем все элементы для этого столбца
        m_proxyModel->setColumnFilters(column, QStringList());
        
        menu.update();
        menu.repaint();
    });

    // Показываем меню и обрабатываем результат
    bool continueLoop = true;
    int maxIterations = 100; // Защита от бесконечного цикла
    int iteration = 0;
    
    while (continueLoop && iteration < maxIterations) {
        iteration++;
        QAction *selected = menu.exec(ui->tableView->horizontalHeader()->mapToGlobal(pos));

        if (!selected) {
            // Пользователь кликнул вне меню - выходим
            continueLoop = false;
        } else if (selected->data().toString() == "select_all_button" || 
                   selected->data().toString() == "deselect_all_button") {
            // Это кнопка управления - не закрываем меню, продолжаем цикл
            // Действие уже выполнено через connect
            continue;
        } else if (valueActions.contains(selected)) {
            // Выбран элемент фильтра - применяем фильтр и закрываем меню
            QStringList selectedValues;
            for (QAction *action : valueActions) {
                if (action->isChecked()) {
                    selectedValues.append(action->data().toString());
                }
            }

            if (selectedValues.isEmpty()) {
                m_proxyModel->setColumnFilters(column, QStringList());
            } else if (selectedValues.size() == valueActions.size()) {
                m_proxyModel->clearColumnFilters(column);
            } else {
                m_proxyModel->setColumnFilters(column, selectedValues);
            }
            continueLoop = false;
        } else {
            // Неизвестное действие - выходим
            continueLoop = false;
        }
    }
    
    // Проверяем, не был ли достигнут лимит итераций
    if (iteration >= maxIterations) {
        qWarning() << "Filter dialog: maximum iterations reached, forcing exit";
    }
}

void MainWindow::onTableSelectionChanged()
{
    // Предотвращаем рекурсивные вызовы
    if (m_searchInProgress) {
        return;
    }
    
    // Проверяем валидность моделей перед выполнением операций
    if (!isModelValid()) {
        return;
    }
    
    // Проверяем валидность selection model
    if (!ui->tableView->selectionModel()) {
        return;
    }
    
    try {
        // Обработка изменения выделения для информационных целей
        QModelIndexList selection = ui->tableView->selectionModel()->selectedColumns();
        if (!selection.isEmpty()) {
            int column = selection.first().column();
            
            // Проверяем валидность номера столбца
            if (column >= 0 && column < m_model->columnCount()) {
                // ТОЛЬКО обновляем статус, НЕ применяем фильтр автоматически
                QVariant headerData = m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole);
                QString columnName = headerData.isValid() ? headerData.toString() : QString("Column %1").arg(column);
                ui->statusbar->showMessage(tr("Selected column: %1").arg(columnName), 2000);
            } else {
                qWarning() << "Invalid column selected:" << column;
                ui->statusbar->showMessage(tr("Invalid column selected"), 2000);
            }
        } else {
            ui->statusbar->showMessage(tr("No column selected"), 2000);
        }
    } catch (const std::exception &e) {
        qWarning() << "Error in onTableSelectionChanged:" << e.what();
        ui->statusbar->showMessage(tr("Selection error"), 2000);
    }
}

void MainWindow::setupTable()
{
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->tableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableView->setDragEnabled(true);
    ui->tableView->setAcceptDrops(true);
    ui->tableView->setDropIndicatorShown(true);
    ui->tableView->setDragDropMode(QAbstractItemView::DragDrop);

    // Настройка растягивания колонок
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    
    // Разрешаем выбор столбцов через заголовки
    ui->tableView->horizontalHeader()->setSectionsClickable(true);
    connect(ui->tableView->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &MainWindow::onHeaderClicked);
}

void MainWindow::setupMenu()
{
    // Контекстное меню
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested, [this](const QPoint &pos) {
        QMenu menu(this);
        menu.addAction(ui->actionAdd);
        menu.addAction(ui->actionEdit);
        menu.addAction(ui->actionDelete);
        menu.addSeparator();
        menu.addAction(ui->actionFind);
        menu.exec(ui->tableView->viewport()->mapToGlobal(pos));
    });
}

void MainWindow::createLanguageMenu()
{
    m_languageMenu = new QMenu(tr("Language"), this);
    ui->menuSettings->addMenu(m_languageMenu);

    QAction *englishAction = m_languageMenu->addAction("English");
    QAction *russianAction = m_languageMenu->addAction("Русский");
    QAction *germanAction = m_languageMenu->addAction("Deutsch");

    connect(englishAction, &QAction::triggered, this, &MainWindow::switchToEnglish);
    connect(russianAction, &QAction::triggered, this, &MainWindow::switchToRussian);
    connect(germanAction, &QAction::triggered, this, &MainWindow::switchToGerman);
}

void MainWindow::setupDragAndDrop()
{
    setAcceptDrops(true);
    ui->tableView->setAcceptDrops(true);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // Загрузка языка
    QString lang = settings.value("language", "en").toString();
    if (lang == "ru") {
        switchToRussian();
    } else if (lang == "de") {
        switchToGerman();
    } else {
        switchToEnglish();
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    // Языковые настройки сохраняются в методах переключения языков
}

void MainWindow::retranslateUi()
{
    ui->retranslateUi(this);
    if (m_languageMenu) {
        m_languageMenu->setTitle(tr("Language"));
    }
    ui->tableView->reset();
}

void MainWindow::refreshView()
{
    if (!m_model || !m_proxyModel) {
        qWarning() << "Model or proxy model is null";
        return;
    }
    
    // Предотвращаем рекурсивные вызовы
    if (m_searchInProgress) {
        return;
    }

    try {
        // Блокируем сигналы во время обновления для избежания рекурсивных вызовов
        m_searchInProgress = true;
        ui->tableView->blockSignals(true);
        m_proxyModel->blockSignals(true);
        
        // Сохраняем текущий выбор
        QModelIndex currentIndex = ui->tableView->currentIndex();

        // Сбрасываем фильтры и пересортировываем
        m_proxyModel->invalidate();

        // Обновляем модель - эмитируем сигнал о том, что данные изменились
        emit m_model->layoutChanged();

        // Автоматически подгоняем размеры колонок
        ui->tableView->resizeColumnsToContents();

        // Устанавливаем минимальную ширину для колонки ID
        int idColumnWidth = ui->tableView->columnWidth(0);
        if (idColumnWidth < 80) {
            ui->tableView->setColumnWidth(0, 80);
        }

        // Ограничиваем максимальную ширину колонок для лучшего отображения
        for (int i = 0; i < m_model->columnCount(); ++i) {
            int width = ui->tableView->columnWidth(i);
            if (width > 200) {
                ui->tableView->setColumnWidth(i, 200);
            }
        }

        // Восстанавливаем выбор, если это возможно
        if (currentIndex.isValid() && m_proxyModel->rowCount() > 0) {
            if (currentIndex.row() < m_proxyModel->rowCount()) {
                ui->tableView->setCurrentIndex(currentIndex);
            } else {
                // Выбираем последнюю строку, если предыдущая была удалена
                QModelIndex lastIndex = m_proxyModel->index(m_proxyModel->rowCount() - 1, 0);
                ui->tableView->setCurrentIndex(lastIndex);
            }
        }

        // Обновляем статусную строку
        int totalRows = m_model->rowCount();
        int visibleRows = m_proxyModel->rowCount();

        if (totalRows == visibleRows) {
            ui->statusbar->showMessage(tr("Total records: %1").arg(totalRows));
        } else {
            ui->statusbar->showMessage(tr("Showing %1 of %2 records").arg(visibleRows).arg(totalRows));
        }

        qDebug() << "View refreshed successfully. Total:" << totalRows << "Visible:" << visibleRows;

    } catch (const std::exception &e) {
        qWarning() << "Error refreshing view:" << e.what();
        ui->statusbar->showMessage(tr("Error refreshing view"), 3000);
    }
    
    // Разблокируем сигналы после завершения обновления
    ui->tableView->blockSignals(false);
    m_proxyModel->blockSignals(false);
    m_searchInProgress = false;
}

void MainWindow::updateWindowTitle()
{
    QString title = tr("Construction Equipment Manager");
    if (!m_currentFile.isEmpty()) {
        title += " - " + QFileInfo(m_currentFile).fileName();
    }
    setWindowTitle(title);
}

bool MainWindow::maybeSave()
{
    return true;
}

void MainWindow::on_actionNew_triggered()
{
    // Если есть открытый файл, спрашиваем о новом окне
    if (!m_currentFile.isEmpty() || m_model->rowCount() > 0) {
        int ret = QMessageBox::question(this, tr("New File"),
                                        tr("Create in new window?"),
                                        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                        QMessageBox::No);
        
        if (ret == QMessageBox::Cancel) return;
        
        if (ret == QMessageBox::Yes) {
            // Создать в новом окне
            openNewWindow();
            return;
        }
        // Иначе создаем в текущем окне
    }
    
    if (!maybeSave()) return;

    try {
        m_model->createNew();
        m_currentFile.clear();
        refreshView();
        updateWindowTitle();
        ui->statusbar->showMessage(tr("New file created"), 2000);
    } catch (const std::exception &e) {
        QMessageBox::critical(this, tr("Error"), e.what());
    }
}

void MainWindow::openNewWindow(const QString &filename)
{
    MainWindow *newWindow = new MainWindow();
    newWindow->show();
    
    if (!filename.isEmpty()) {
        try {
            newWindow->m_model->loadFromFile(filename);
            newWindow->m_currentFile = filename;
            newWindow->refreshView();
            newWindow->updateWindowTitle();
        } catch (const std::exception &e) {
            QMessageBox::critical(newWindow, tr("Error"), e.what());
        }
    }
}

bool MainWindow::validateCellData(int column, const QVariant &value, const QModelIndex &index) const
{
    return m_model->validateData(column, value);
}

void MainWindow::on_actionOpen_triggered()
{
    // Предложение: открыть в новом окне или текущем
    int ret = QMessageBox::question(this, tr("Open File"),
                                    tr("Open in new window?"),
                                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                                    QMessageBox::No);
    
    if (ret == QMessageBox::Cancel) return;
    
    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), "",
                                                    tr("Database Files (*.db)"));
    if (filename.isEmpty()) return;

    if (ret == QMessageBox::Yes) {
        // Открыть в новом окне
        openNewWindow(filename);
    } else {
        // Открыть в текущем окне
        if (!maybeSave()) return;
        
        try {
            m_model->loadFromFile(filename);
            m_currentFile = filename;
            refreshView();
            updateWindowTitle();
            ui->statusbar->showMessage(tr("File loaded"), 2000);
        } catch (const std::exception &e) {
            QMessageBox::critical(this, tr("Error"), e.what());
        }
    }
}

void MainWindow::on_actionSave_triggered()
{
    if (m_currentFile.isEmpty()) {
        on_actionSaveAs_triggered();
        return;
    }

    try {
        m_model->saveToFile(m_currentFile);
        ui->statusbar->showMessage(tr("File saved"), 2000);
    } catch (const std::exception &e) {
        QMessageBox::critical(this, tr("Error"), e.what());
    }
}

void MainWindow::on_actionSaveAs_triggered()
{
    QString filename = QFileDialog::getSaveFileName(this, tr("Save File"), "",
                                                    tr("Database Files (*.db)"));
    if (filename.isEmpty()) return;

    if (!filename.endsWith(".db", Qt::CaseInsensitive)) {
        filename += ".db";
    }

    try {
        m_model->saveToFile(filename);
        m_currentFile = filename;
        updateWindowTitle();
        ui->statusbar->showMessage(tr("File saved"), 2000);
    } catch (const std::exception &e) {
        QMessageBox::critical(this, tr("Error"), e.what());
    }
}

void MainWindow::on_actionAdd_triggered()
{
    AddEditDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        try {
            m_model->addEquipment(dialog.getEquipment());
            ui->statusbar->showMessage(tr("Equipment added successfully"), 2000);
        } catch (const std::exception &e) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Failed to add equipment: %1").arg(e.what()));
        }
    }
}

void MainWindow::on_actionEdit_triggered()
{
    if (!isModelValid()) {
        QMessageBox::critical(this, tr("Error"), tr("Application is not properly initialized."));
        return;
    }

    // Получаем текущий выделенный индекс (любая ячейка)
    QModelIndex currentIndex = ui->tableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Edit"), tr("Please select a cell to edit."));
        return;
    }

    // Получаем исходный индекс
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    if (!sourceIndex.isValid()) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid selection."));
        return;
    }
    
    int sourceRow = sourceIndex.row();

    if (sourceRow < 0 || sourceRow >= m_model->rowCount()) {
        QMessageBox::warning(this, tr("Error"), tr("Selected row is out of range."));
        return;
    }

    Equipment eq = m_model->getEquipmentAt(sourceRow);

    if (eq.id() < 0) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to get equipment data."));
        return;
    }

    AddEditDialog dialog(this);
    dialog.setWindowTitle(tr("Edit Equipment"));
    dialog.setEquipment(eq);
    if (dialog.exec() == QDialog::Accepted) {
        try {
            m_model->updateEquipment(eq.id(), dialog.getEquipment());
            ui->statusbar->showMessage(tr("Equipment updated successfully"), 2000);
        } catch (const std::exception &e) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Failed to update equipment: %1").arg(e.what()));
        }
    }
}

void MainWindow::on_actionDelete_triggered()
{
    if (!isModelValid()) {
        QMessageBox::critical(this, tr("Error"), tr("Application is not properly initialized."));
        return;
    }

    // Получаем текущий выделенный индекс (любая ячейка)
    QModelIndex currentIndex = ui->tableView->currentIndex();
    if (!currentIndex.isValid()) {
        QMessageBox::information(this, tr("Delete"), tr("Please select a cell to delete."));
        return;
    }

    // Получаем исходный индекс
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    if (!sourceIndex.isValid()) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid selection."));
        return;
    }

    int sourceRow = sourceIndex.row();

    // Проверяем, что индекс в допустимых пределах
    if (sourceRow < 0 || sourceRow >= m_model->rowCount()) {
        QMessageBox::warning(this, tr("Error"), tr("Selected row is out of range."));
        return;
    }

    Equipment eq = m_model->getEquipmentAt(sourceRow);

    // Проверяем, что получили валидный объект
    if (eq.id() < 0) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to get equipment data."));
        return;
    }

    int ret = QMessageBox::question(this, tr("Delete Equipment"),
                                    tr("Are you sure you want to delete equipment?")
                                        .arg(eq.model()).arg(eq.regNumber()),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);
    qDebug() << "Translation for delete message:" << tr("Are you sure you want to delete equipment:\n%1 (%2)?");

    if (ret == QMessageBox::Yes) {
        try {
            m_model->removeEquipment(eq.id());
            ui->statusbar->showMessage(tr("Equipment deleted successfully"), 2000);
        } catch (const std::exception &e) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("Failed to delete equipment: %1").arg(e.what()));
        }
    }
}

bool MainWindow::isModelValid() const
{
    return m_model != nullptr && m_proxyModel != nullptr && ui->tableView != nullptr;
}

void MainWindow::on_actionFind_triggered()
{
    // Проверка валидности моделей перед выполнением поиска
    if (!isModelValid()) {
        qWarning() << "Models are not valid for search operation";
        return;
    }
    
    // Предотвращаем рекурсивные вызовы
    if (m_searchInProgress) {
        return;
    }
    
    // Устанавливаем фокус на поле поиска
    ui->searchEdit->setFocus();
    ui->searchEdit->selectAll();
    
    // Если в поле поиска есть текст, выполняем поиск немедленно
    QString term = ui->searchEdit->text();
    if (!term.isEmpty()) {
        performDelayedSearch();
    }
}

void MainWindow::on_actionPrint_triggered()
{
    PrintService::printEquipmentList(m_model->getAllEquipment(), this);
}

void MainWindow::on_actionShowChart_triggered()
{
    ChartDialog dialog(m_model->getAllEquipment(), this);
    dialog.exec();
}

void MainWindow::on_actionSettings_triggered()
{
    m_settings->exec();
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::switchToEnglish()
{
    // Удаляем текущие переводчики
    if (s_translator) {
        qApp->removeTranslator(s_translator);
        delete s_translator;
        s_translator = nullptr;
    }
    if (s_translator2) {
        qApp->removeTranslator(s_translator2);
        delete s_translator2;
        s_translator2 = nullptr;
    }

    // 1. Загружаем системные переводы Qt (Yes/No/Cancel и т.д.)
    s_translator2 = new QTranslator();
    if (s_translator2->load("qt_en", QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        qApp->installTranslator(s_translator2);
    } else {
        qDebug() << "Failed to load system Qt translations from:"
                 << QLibraryInfo::path(QLibraryInfo::TranslationsPath);
        delete s_translator2;
        s_translator2 = nullptr;
    }

    // 2. Загружаем собственные переводы приложения
    s_translator = new QTranslator();
    if (s_translator->load("app_en", ":/i18n")) {
        qApp->installTranslator(s_translator);
    } else {
        // Альтернативный путь - из папки рядом с исполняемым файлом
        QString appPath = QCoreApplication::applicationDirPath() + "/translations/app_en.qm";
        if (s_translator->load(appPath)) {
            qApp->installTranslator(s_translator);
        } else {
            qDebug() << "Failed to load app translations from:"
                     << ":/i18n and" << appPath;
            delete s_translator;
            s_translator = nullptr;
        }
    }

    // Сохраняем настройку языка
    QSettings().setValue("language", "en");

    // Обновляем интерфейс всех окон
    for (MainWindow* window : s_openWindows) {
        if (window) {
            window->retranslateUi();
        }
    }
}

void MainWindow::switchToRussian()
{
    // Удаляем текущие переводчики
    if (s_translator) {
        qApp->removeTranslator(s_translator);
        delete s_translator;
        s_translator = nullptr;
    }
    if (s_translator2) {
        qApp->removeTranslator(s_translator2);
        delete s_translator2;
        s_translator2 = nullptr;
    }

    // 1. Загружаем системные переводы Qt (Yes/No/Cancel и т.д.)
    s_translator2 = new QTranslator();
    if (s_translator2->load("qt_ru", QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        qApp->installTranslator(s_translator2);
    } else {
        qDebug() << "Failed to load system Qt translations from:"
                 << QLibraryInfo::path(QLibraryInfo::TranslationsPath);
        delete s_translator2;
        s_translator2 = nullptr;
    }

    // 2. Загружаем собственные переводы приложения
    s_translator = new QTranslator();
    qDebug() << "Attempting to load Russian translations from :/i18n";
    if (s_translator->load("app_ru", ":/i18n")) {
        qApp->installTranslator(s_translator);
        qDebug() << "Successfully loaded Russian translations from resources";
    } else {
        qDebug() << "Failed to load from resources, trying file path";
        // Альтернативный путь - из папки рядом с исполняемым файлом
        QString appPath = QCoreApplication::applicationDirPath() + "/translations/app_ru.qm";
        qDebug() << "Trying to load from:" << appPath;
        if (s_translator->load(appPath)) {
            qApp->installTranslator(s_translator);
            qDebug() << "Successfully loaded Russian translations from file";
        } else {
            qDebug() << "Failed to load app translations from:"
                     << ":/i18n and" << appPath;
            delete s_translator;
            s_translator = nullptr;
        }
    }

    // Сохраняем настройку языка
    QSettings().setValue("language", "ru");

    // Обновляем интерфейс всех окон
    for (MainWindow* window : s_openWindows) {
        if (window) {
            window->retranslateUi();
        }
    }
}

void MainWindow::switchToGerman()
{
    // Удаляем текущие переводчики
    if (s_translator) {
        qApp->removeTranslator(s_translator);
        delete s_translator;
        s_translator = nullptr;
    }
    if (s_translator2) {
        qApp->removeTranslator(s_translator2);
        delete s_translator2;
        s_translator2 = nullptr;
    }

    // 1. Загружаем системные переводы Qt (Yes/No/Cancel и т.д.)
    s_translator2 = new QTranslator();
    if (s_translator2->load("qt_de", QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        qApp->installTranslator(s_translator2);
    } else {
        qDebug() << "Failed to load system Qt translations from:"
                 << QLibraryInfo::path(QLibraryInfo::TranslationsPath);
        delete s_translator2;
        s_translator2 = nullptr;
    }

    // 2. Загружаем собственные переводы приложения
    s_translator = new QTranslator();
    if (s_translator->load("app_de", ":/i18n")) {
        qApp->installTranslator(s_translator);
    } else {
        // Альтернативный путь - из папки рядом с исполняемым файлом
        QString appPath = QCoreApplication::applicationDirPath() + "/translations/app_de.qm";
        if (s_translator->load(appPath)) {
            qApp->installTranslator(s_translator);
        } else {
            qDebug() << "Failed to load app translations from:"
                     << ":/i18n and" << appPath;
            delete s_translator;
            s_translator = nullptr;
        }
    }

    // Сохраняем настройку языка
    QSettings().setValue("language", "de");

    // Обновляем интерфейс всех окон
    for (MainWindow* window : s_openWindows) {
        if (window) {
            window->retranslateUi();
        }
    }
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(ui->actionAdd);
    menu.addAction(ui->actionEdit);
    menu.addAction(ui->actionDelete);
    menu.addSeparator();
    menu.addAction(ui->actionFind);
    menu.exec(event->globalPos());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (!mimeData->hasUrls()) return;

    QStringList files;
    for (const QUrl &url : mimeData->urls()) {
        files << url.toLocalFile();
    }

    if (!files.isEmpty()) {
        try {
            m_model->loadFromFile(files.first());
            m_currentFile = files.first();
            refreshView();
            updateWindowTitle();
        } catch (const std::exception &e) {
            QMessageBox::critical(this, tr("Error"), e.what());
        }
    }
}

void MainWindow::onHeaderClicked(int logicalIndex)
{
    // Предотвращаем рекурсивные вызовы
    if (m_searchInProgress) {
        return;
    }
    
    // Проверяем валидность моделей
    if (!isModelValid()) {
        return;
    }
    
    // Проверяем валидность индекса столбца
    if (logicalIndex < 0 || logicalIndex >= m_model->columnCount()) {
        qWarning() << "Invalid column index clicked:" << logicalIndex;
        return;
    }
    
    try {
        // Выделяем весь столбец при нажатии на заголовок
        ui->tableView->selectColumn(logicalIndex);
        
        // Устанавливаем фильтр для поиска в выбранном столбце БЕЗ немедленного применения
        QString searchText = ui->searchEdit->text();
        
        // Безопасно получаем заголовок столбца
        QVariant headerData = m_model->headerData(logicalIndex, Qt::Horizontal, Qt::DisplayRole);
        QString columnName = headerData.isValid() ? headerData.toString() : QString("Column %1").arg(logicalIndex);
        
        if (!searchText.isEmpty()) {
            ui->statusbar->showMessage(tr("Click search to filter in column: %1").arg(columnName), 3000);
        } else {
            ui->statusbar->showMessage(tr("Column selected: %1").arg(columnName), 2000);
        }
        
    } catch (const std::exception &e) {
        qWarning() << "Error in onHeaderClicked:" << e.what();
        ui->statusbar->showMessage(tr("Column selection error"), 2000);
    }
}

void MainWindow::performDelayedSearch()
{
    // Проверка валидности моделей перед выполнением поиска
    if (!isModelValid()) {
        qWarning() << "Models are not valid for search operation";
        return;
    }
    
    if (!m_proxyModel) {
        qWarning() << "performDelayedSearch: m_proxyModel is null! Cannot perform search.";
        ui->statusbar->showMessage(tr("Search error: Proxy model not initialized"), 3000);
        return;
    }

    // Устанавливаем флаг для предотвращения рекурсии
    if (m_searchInProgress) {
        return;
    }
    
    m_searchInProgress = true;

    QString term = ui->searchEdit->text();
    qDebug() << "performDelayedSearch: Starting search for term:" << term;

    try {
        // Блокируем сигналы proxy model во время операции
        m_proxyModel->blockSignals(true);
        
        // Определяем, в каком столбце искать
        int searchColumn = -1; // По умолчанию ищем во всех столбцах
        
        if (ui->tableView->selectionModel()) {
            QModelIndexList selection = ui->tableView->selectionModel()->selectedColumns();
            if (!selection.isEmpty()) {
                int column = selection.first().column();
                // Проверяем валидность номера столбца
                if (column >= 0 && column < m_model->columnCount()) {
                    searchColumn = column;
                }
            }
        }
        
        // Устанавливаем столбец для поиска
        m_proxyModel->setFilterKeyColumn(searchColumn);
        
        // Применяем фильтр
        m_proxyModel->setFilterFixedString(term);
        
        // Разблокируем сигналы
        m_proxyModel->blockSignals(false);
        
        // Обновляем статус
        if (searchColumn >= 0) {
            QVariant headerData = m_model->headerData(searchColumn, Qt::Horizontal, Qt::DisplayRole);
            QString columnName = headerData.isValid() ? headerData.toString() : QString("Column %1").arg(searchColumn);
            ui->statusbar->showMessage(tr("Searching in column: %1").arg(columnName), 2000);
        } else {
            ui->statusbar->showMessage(tr("Searching in all columns"), 2000);
        }
        
        qDebug() << "performDelayedSearch: Search completed successfully";
        
    } catch (const std::exception &e) {
        qWarning() << "Error during search operation:" << e.what();
        ui->statusbar->showMessage(tr("Search error occurred"), 3000);
        // Разблокируем сигналы в случае ошибки
        m_proxyModel->blockSignals(false);
    }
    
    m_searchInProgress = false;
}
