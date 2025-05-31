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
    // Проверяем обычный текстовый фильтр
    if (!QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent)) {
        return false;
    }
    
    // Проверяем столбцовые фильтры
    for (auto it = m_columnFilters.constBegin(); it != m_columnFilters.constEnd(); ++it) {
        int column = it.key();
        const QStringList &allowedValues = it.value();
        
        // Если список фильтров пустой, но он существует, значит скрываем все
        if (allowedValues.isEmpty()) {
            return false;
        }
        
        // Если есть фильтры, проверяем вхождение
        QModelIndex index = sourceModel()->index(sourceRow, column, sourceParent);
        QString value = sourceModel()->data(index).toString();
        
        if (!allowedValues.contains(value)) {
            return false;
        }
    }
    
    return true;
}

// Определение статического члена (п.16 ТЗ)
QTranslator *MainWindow::s_translator = nullptr;
QList<MainWindow*> MainWindow::s_openWindows;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new EquipmentModel(this))
    , m_proxyModel(new MultiSelectFilterProxyModel(this))
    , m_settings(new Settings(this))
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

    // Подключение сигналов для поиска в реальном времени (п.17, п.22 ТЗ)
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &MainWindow::on_actionFind_triggered);
    
    // Подключение сигнала изменения выделения
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onTableSelectionChanged);

    // Статус бар
    ui->statusbar->showMessage(tr("Ready"));
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
    selectAll->setData("control_button");
    deselectAll->setData("control_button");

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
    connect(deselectAll, &QAction::triggered, [&valueActions, &menu]() {
        for (QAction *action : valueActions) {
            action->setChecked(false);
        }
        //
    });

    // Показываем меню и обрабатываем результат
    bool continueLoop = true;
    while (continueLoop) {
        QAction *selected = menu.exec(ui->tableView->horizontalHeader()->mapToGlobal(pos));

        if (!selected) {
            // Пользователь кликнул вне меню - выходим
            continueLoop = false;
        } else if (selected->data().toString() == "control_button") {
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
}

void MainWindow::onTableSelectionChanged()
{
    // Обработка изменения выделения для поиска по столбцу
    QModelIndexList selection = ui->tableView->selectionModel()->selectedColumns();
    if (!selection.isEmpty()) {
        int column = selection.first().column();
        m_proxyModel->setFilterKeyColumn(column);
        // Применяем текущий поисковый запрос к выбранному столбцу
        QString searchText = ui->searchEdit->text();
        if (!searchText.isEmpty()) {
            m_proxyModel->setFilterFixedString(searchText);
        }
        ui->statusbar->showMessage(tr("Search in column: %1").arg(m_model->headerData(column, Qt::Horizontal, Qt::DisplayRole).toString()), 2000);
    } else {
        // Если нет выделенных столбцов, ищем по всем
        m_proxyModel->setFilterKeyColumn(-1);
        ui->statusbar->showMessage(tr("Search in all columns"), 2000);
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

    try {
        // Сохраняем текущий выбор
        QModelIndex currentIndex = ui->tableView->currentIndex();

        // Сбрасываем фильтры и пересортировываем
        m_proxyModel->invalidate();

        // Обновляем модель
        m_model->layoutChanged();

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
        m_model->addEquipment(dialog.getEquipment());
        refreshView();
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
                                    tr("Are you sure you want to delete equipment:\n%1 (%2)?")
                                        .arg(eq.model()).arg(eq.regNumber()),
                                    QMessageBox::Yes | QMessageBox::No,
                                    QMessageBox::No);

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
    // Поиск по выделенному столбцу или по всем столбцам
    QString term = ui->searchEdit->text();
    
    // Если есть выделенные столбцы, ищем в них
    QModelIndexList selection = ui->tableView->selectionModel()->selectedColumns();
    if (!selection.isEmpty()) {
        m_proxyModel->setFilterKeyColumn(selection.first().column());
    } else {
        m_proxyModel->setFilterKeyColumn(-1); // Поиск по всем столбцам
    }
    
    m_proxyModel->setFilterFixedString(term);
    ui->searchEdit->setFocus();
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
    // Удаляем текущий переводчик
    if (s_translator) {
        qApp->removeTranslator(s_translator);
        delete s_translator;
    }

    // Создаем новый переводчик для английского языка (п.16 ТЗ)
    s_translator = new QTranslator();
    if (s_translator->load("translations/app_en.qm")) {
        qApp->installTranslator(s_translator);
        qDebug() << "English translation loaded successfully";
    } else {
        qDebug() << "Failed to load English translation";
        delete s_translator;
        s_translator = nullptr;
    }

    // Сохраняем настройку языка (п.20 ТЗ)
    QSettings().setValue("language", "en");

    // Обновляем интерфейс
    retranslateUi();
}


void MainWindow::switchToRussian()
{
    // Удаляем текущий переводчик
    if (s_translator) {
        qApp->removeTranslator(s_translator);
        delete s_translator;
    }
    
    // Создаем новый переводчик для русского языка (п.16 ТЗ)
    s_translator = new QTranslator();
    if (s_translator->load("translations/app_ru.qm")) {
        qApp->installTranslator(s_translator);
        qDebug() << "Russian translation loaded successfully";
    } else {
        qDebug() << "Failed to load Russian translation";
        delete s_translator;
        s_translator = nullptr;
    }
    
    // Сохраняем настройку языка (п.20 ТЗ)
    QSettings().setValue("language", "ru");
    
    // Обновляем интерфейс
    retranslateUi();
}

void MainWindow::switchToGerman()
{
    // Удаляем текущий переводчик
    if (s_translator) {
        qApp->removeTranslator(s_translator);
        delete s_translator;
    }
    
    // Создаем новый переводчик для немецкого языка (п.16 ТЗ)
    s_translator = new QTranslator();
    if (s_translator->load("translations/app_de.qm")) {
        qApp->installTranslator(s_translator);
        qDebug() << "German translation loaded successfully";
    } else {
        qDebug() << "Failed to load German translation";
        delete s_translator;
        s_translator = nullptr;
    }
    
    // Сохраняем настройку языка (п.20 ТЗ)
    QSettings().setValue("language", "de");
    
    // Обновляем интерфейс
    retranslateUi();
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
    // Выделяем весь столбец при нажатии на заголовок
    ui->tableView->selectColumn(logicalIndex);
    
    // Устанавливаем фильтр для поиска в выбранном столбце
    m_proxyModel->setFilterKeyColumn(logicalIndex);
    
    // Применяем текущий поисковый запрос к выбранному столбцу
    QString searchText = ui->searchEdit->text();
    if (!searchText.isEmpty()) {
        m_proxyModel->setFilterFixedString(searchText);
    }
    
    // Обновляем статус-бар
    ui->statusbar->showMessage(tr("Search in column: %1").arg(m_model->headerData(logicalIndex, Qt::Horizontal, Qt::DisplayRole).toString()), 2000);
}
