#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "addeditdialog.h"
#include "aboutdialog.h"
#include "settings.h"
#include "printservice.h"
#include "chartdialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QMenu>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QApplication>

// Определение статического члена (п.16 ТЗ)
QTranslator *MainWindow::s_translator = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_model(new EquipmentModel(this))
    , m_proxyModel(new QSortFilterProxyModel(this))
    , m_settings(new Settings(this))
{
    ui->setupUi(this);

    // Настройка модели и прокси
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterKeyColumn(0);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    ui->tableView->setModel(m_proxyModel);
    ui->tableView->setSortingEnabled(true);
    ui->tableView->sortByColumn(0, Qt::AscendingOrder);

    // Настройка интерфейса
    setupTable();
    setupMenu();
    createLanguageMenu();
    loadSettings();
    setupDragAndDrop();

    // Подключение сигналов для поиска в реальном времени (п.17, п.22 ТЗ)
    connect(ui->searchEdit, &QLineEdit::textChanged, this, &MainWindow::on_actionFind_triggered);
    connect(ui->searchComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::on_actionFind_triggered);

    // Статус бар
    ui->statusbar->showMessage(tr("Ready"));
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::setupTable()
{
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView->setDragEnabled(true);
    ui->tableView->setAcceptDrops(true);
    ui->tableView->setDropIndicatorShown(true);
    ui->tableView->setDragDropMode(QAbstractItemView::DragDrop);

    // Настройка растягивания колонок
    ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
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
    // Проверка необходимости сохранения
    return true;
}

void MainWindow::on_actionOpen_triggered()
{
    if (!maybeSave()) return;

    QString filename = QFileDialog::getOpenFileName(this, tr("Open File"), "",
                                                    tr("Database Files (*.db)"));
    if (filename.isEmpty()) return;

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

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) return;

    QModelIndex sourceIndex = m_proxyModel->mapToSource(selection.first());
    if (!sourceIndex.isValid()) return;
    int sourceRow = sourceIndex.row();

    if (sourceRow < 0 || sourceRow >= m_model->rowCount()) return;

    Equipment eq = m_model->getEquipmentAt(sourceRow);

    if (eq.id() < 0) return;

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

    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::information(this, tr("Delete"), tr("Please select a row to delete."));
        return;
    }

    // Получаем индекс в исходной модели
    QModelIndex sourceIndex = m_proxyModel->mapToSource(selection.first());
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

    // Подтверждение удаления
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
    // 1. Устанавливаем фокус и выделяем весь текст для быстрой замены
    ui->searchEdit->setFocus();

    // 2. Выполняем фильтрацию
    QString term = ui->searchEdit->text();
    int column = ui->searchComboBox->currentIndex();

    m_proxyModel->setFilterKeyColumn(column);
    m_proxyModel->setFilterFixedString(term);
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
