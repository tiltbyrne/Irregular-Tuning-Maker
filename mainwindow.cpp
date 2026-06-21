#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>
#include <QDebug>
#include <QFileDialog>
#include <QDockWidget>
#include <QMessageBox>
#include <QSpinBox>
#include <QClipboard>
#include <QShortcut>
#include <QDebug>
#include <QThread>
#include <QToolTip>

#include "weights.h"
#include "settings.h"
#include "dbManager.h"
#include "utilities.h"
#include "customheaderview.h"
#include "fractionParser.h"
#include "scale.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , weightFunctions(populateWeightFunctionsMap())
    , scaleSpace(settings::scaleSpaceName, dbManager::openDatabase(settings::scaleSpaceName)->loadPattern(), this)//if w is constructed after dbManager initialises then I think it's impossible for this to return null
    , currentFileUrl(QUrl::fromLocalFile(dbUtils::makeUrlString(settings::scaleSpaceName, dbUtils::databaseDirectory)))
    , sizeModel(new QStandardItemModel(scaleSpace.size(), scaleSpace.size(), this))
    , weightModel(new QStandardItemModel(scaleSpace.size(), scaleSpace.size(), this))
    , displayModeGroup(new QButtonGroup(this))
    , sizeWeightModeGroup(new QButtonGroup(this))
{
    ui->setupUi(this);

    QApplication::instance()->installEventFilter(this);
    initialiseSettings();

    initialiseDisplaySettings();
    initialiseMakeCancelButtons();
    initialiseSizeWeightRatioGroup();
    initialisePrecisionSpinBox();
    initialiseScaleSpacesCombo();
    initialiseRangeSpinBox();
    initialiseWeightFunctionsCombo();
    initialiseCutoffDial();
    initialiseAttenuationSlider();
    initialiseSaveSubScaleButtons();
    initialiseTable();
}

MainWindow::~MainWindow()
{
    tuneScaleCancelRequested = true;

    future.waitForFinished();

    delete ui;
}

void MainWindow::handleScaleSpaceActivated(QString selection)
{
    if (selection == scaleSpace.getName() &&
        selection != settings::customScaleSpaceName)
        return;

    if (selection == settings::customScaleSpaceName) //opening a custom scale space
    {
        const QFileInfo file{ QFileDialog::getOpenFileName(this,
                                                           "Open File",
                                                           QDir::currentPath(),
                                                           "Scale Spaces (*." + dbUtils::filetypeName + ")") };

        if (file.exists())
        {
            const auto newDatabase{ dbManager::openDatabase(file.baseName(), file.absolutePath()) };

            const auto currentInfo{ currentFileUrl.toLocalFile() };

            if (currentInfo != file.filePath())
            {
                currentFileUrl = QUrl::fromLocalFile(file.filePath());
                emit currentUrlChanged(currentFileUrl);
            }

            changeTable(file.baseName(), newDatabase);

            return;
        }
    }
    else //opening a built-in scale space
    {
        const auto newDatabase{ dbManager::openDatabase(selection) };

        const auto currentInfo{ currentFileUrl.toLocalFile() };

        const auto file{ QFileInfo(dbUtils::makeUrlString(selection, dbUtils::databaseDirectory)) };

        if (currentInfo != file.filePath())
        {
            currentFileUrl = QUrl::fromLocalFile(file.filePath());
            emit currentUrlChanged(currentFileUrl);
        }

        changeTable(selection, newDatabase);

        return;
    }

    if (selection != settings::customScaleSpaceName)
        ui->scaleSpaceCombo->setCurrentIndex(ui->scaleSpaceCombo->findText(scaleSpace.getName()));
}

void MainWindow::handleSizeModelItemChanged(QStandardItem* item)
{
    if (shouldNotProcessSizeChange)
        return;

    shouldNotProcessSizeChange = true;

    const auto row{ item->row() },
               column{ item->column() };

    bool canBeDouble;
    item->text().toDouble(&canBeDouble);
    const auto canBeFraction{ inputCanBeFraction(item->text()) };

    if (!(canBeDouble || canBeFraction) || //invalid
        item->text().isEmpty() || //invalid
        item->text().isNull() || //invalid
        row == column) //is unison
    {
        refreshModelsItemsText(column, row);
        shouldNotProcessSizeChange = false;

        return;
    }

    const auto ratioSize{ intervalSizeAsRatio(canBeFraction ? parseFraction(item->text())
                                                            : qstld(item->text())) };

    if (std::abs(column - row) % scaleSpace.storedSize() == 0) //interval is an 'octave'
    {
        scaleSpace.setIntervalSize(scaleSpace.storedSize(), 0,
                                   std::pow(ratioSize,
                                            static_cast<long double>( 1.L /
                                            (static_cast<long double>(column - row) / static_cast<long double>(scaleSpace.storedSize())))));

        refreshModels();
    }
    else
    {
        auto sizeChange{ ratioSize / scaleSpace.getIntervalSize(column, row) };

        auto noteFrom{ scaleSpace.getBaseNote(row) },
             noteTo{ scaleSpace.getBaseNote(column) };

        if (noteFrom > noteTo)
        {
            std::swap(noteFrom, noteTo);
            sizeChange = 1.L / sizeChange;
        }

        scaleSpace.setIntervalSize(noteTo, noteFrom,
                                   scaleSpace.getIntervalSize(noteTo, noteFrom) * sizeChange);

        const auto scaleSpaceSize{ scaleSpace.storedSize() },
                   modelRows{ sizeModel->rowCount() },
                   modelColumns{ sizeModel->columnCount() };

        const auto origionalNoteTo{ noteTo };
        const auto difference{ noteTo - noteFrom };

        while (noteFrom < modelRows)
        {
            noteTo = origionalNoteTo;

            while (noteTo < modelColumns)
            {
                refreshModelsItemsText(noteTo, noteFrom);
                refreshModelsItemsText(noteFrom, noteTo);

                if (noteTo - noteFrom != difference &&
                    noteFrom + difference < modelColumns &&
                    noteTo - difference < modelRows)
                    refreshModelsItemsText(noteFrom + difference, noteTo - difference);

                noteTo += scaleSpaceSize;
            }

            noteFrom += scaleSpaceSize;
        }
    }

    shouldNotProcessSizeChange = false;
}

void MainWindow::handleWeightModelItemChanged(QStandardItem *item)
{
    if (shouldNotProcessWeightChange)
        return;

    const auto value{ std::clamp(qstld(item->text()), 0.L, 1.L) };

    if (ui->weightFuncCombo->currentText() != settings::customWeightFuncName)
        ui->weightFuncCombo->setCurrentText(settings::customWeightFuncName);

    if (ui->weightFuncCombo->currentText() == settings::customWeightFuncName)
    {
        const auto row{ item->row() };
        const auto column{ item->column() };

        shouldNotProcessWeightChange = true;

        weightModel->item(row, column)->setText(ldtqs(value, ui->precisionSpinBox->value(), true));
        weightModel->item(column, row)->setText(ldtqs(value, ui->precisionSpinBox->value(), true));

        shouldNotProcessWeightChange = false;

        if (row < cashedPreCustomWeightTable.size() &&
            column < cashedPreCustomWeightTable.size())
        {
            cashedPreCustomWeightTable[row][column] = value;
            cashedPreCustomWeightTable[column][row] = value;
        }
    }
}

void MainWindow::refreshModels()
{
    for (auto noteFrom{ 0 }; noteFrom != sizeModel->rowCount(); ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != sizeModel->columnCount(); ++noteTo)
            refreshModelsItemsText(noteTo, noteFrom);
}

void MainWindow::refreshWeightModel()
{
    for (auto noteFrom{ 0 }; noteFrom != sizeModel->rowCount(); ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != sizeModel->columnCount(); ++noteTo)
            refreshWeightItemText(noteTo, noteFrom);
}

void MainWindow::initialiseTable()
{
    populateModels();

    auto* horizontalHeader{ new CustomHeaderView(scaleSpace.size(),
                                                 Qt::Horizontal,
                                                 ui->scaleSpaceTable) };
    ui->scaleSpaceTable->setHorizontalHeader(horizontalHeader);
    ui->scaleSpaceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    auto* verticalHeader{ new CustomHeaderView(scaleSpace.size(),
                                               Qt::Vertical,
                                               ui->scaleSpaceTable) };
    ui->scaleSpaceTable->setVerticalHeader(verticalHeader);
    ui->scaleSpaceTable->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->scaleSpaceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->scaleSpaceTable->setFocusPolicy(Qt::ClickFocus);

    ui->scaleSpaceTable->setModel(sizeModel);
    ui->scaleSpaceTable->installEventFilter(this);

    ui->scaleSpaceTable->viewport()->installEventFilter(this);

    connect(&scaleSpace,
            &ScaleSpace::sizeChanged,
            horizontalHeader,
            &CustomHeaderView::handleScaleSpaceSizeChanged);

    connect(&(this->scaleSpace),
            &ScaleSpace::spaceTooSmall,
            horizontalHeader,
            [horizontalHeader](auto isTooSmall)
            {
                horizontalHeader->enableAction("delNote", !isTooSmall);
            });

    connect(sizeModel,
            &QStandardItemModel::itemChanged,
            this,
            &MainWindow::handleSizeModelItemChanged);

    connect(weightModel,
            &QStandardItemModel::itemChanged,
            this,
            &MainWindow::handleWeightModelItemChanged);

    connect(horizontalHeader,
            &CustomHeaderView::leftClicked,
            this,
            &MainWindow::handleHeaderLeftClicked);

    connect(horizontalHeader,
            &CustomHeaderView::addNote,
            this,
            &MainWindow::handleAddNote);

    connect(horizontalHeader,
            &CustomHeaderView::deleteNote,
            this,
            &MainWindow::handleDeleteNote);

    connect(horizontalHeader,
            &CustomHeaderView::clearSelection,
            this,
            &MainWindow::handleClearSelection);

    connect(verticalHeader,
            &CustomHeaderView::leftClicked,
            this,
            &MainWindow::handleHeaderLeftClicked);

    connect(verticalHeader,
            &CustomHeaderView::addNote,
            this,
            &MainWindow::handleAddNote);

    connect(verticalHeader,
            &CustomHeaderView::deleteNote,
            this,
            &MainWindow::handleDeleteNote);

    connect(verticalHeader,
            &CustomHeaderView::clearSelection,
            this,
            &MainWindow::handleClearSelection);

    connect(&scaleSpace,
            &ScaleSpace::sizeChanged,
            verticalHeader,
            &CustomHeaderView::handleScaleSpaceSizeChanged);

    setSelectionModelBehaviour();

}

void MainWindow::initialiseWeightFunctionsCombo()
{
    ui->weightFuncCombo->addItems(settings::weightFunctionNames);
    ui->weightFuncCombo->addItem(settings::customWeightFuncName);

    connect(ui->weightFuncCombo,
            &QComboBox::currentIndexChanged,
            this,
            &MainWindow::weightFunctionChanged);
}

void MainWindow::initialiseScaleSpacesCombo()
{
    ui->scaleSpaceCombo->addItems(settings::scaleSpaceNames);

    ui->scaleSpaceCombo->setCurrentIndex(ui->scaleSpaceCombo->findText(settings::scaleSpaceName));
    ui->saveButton->setEnabled(false);

    connect(ui->scaleSpaceCombo,
            &QComboBox::textActivated,
            this,
            &MainWindow::handleScaleSpaceActivated);

    connect(this,
            &MainWindow::currentUrlChanged,
            this,
            &MainWindow::handleCurrentUrlChanged);
}

void MainWindow::initialiseCutoffDial()
{
    ui->cutoffDial->setValue(50);
}

void MainWindow::initialiseAttenuationSlider()
{
    connect(ui->attenuationSlider,
            &QDial::valueChanged,
            this,
            [this](auto value) //passes normalised weight to scale
            {
                refreshWeightModel();
            });
}

void MainWindow::initialiseSettings()
{
    setWindowTitle(settings::scaleSpaceName + " - " + globals::appName);
}

bool itemShouldBeAltColour(const int& noteFrom, const int& noteTo, const int& scaleSize)
{
    return posMod(noteFrom / scaleSize, 2) != posMod(noteTo / scaleSize, 2);
}

void MainWindow::handleHeaderLeftClicked(int logicalIndex)
{
    auto selectionModel{ ui->scaleSpaceTable->selectionModel() };

    if (!selectionIsSymetric())
        selectionModel->clearSelection();

    const auto selectionFlag{ selectionModel->rowIntersectsSelection(logicalIndex)
                              ? QItemSelectionModel::Deselect
                              : QItemSelectionModel::Select };

    const auto notesCount{ currentModel()->rowCount() };

    auto selectedNotes{ getSelectedNotes() };
    selectedNotes.push_back(logicalIndex);

    if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        const auto baseIndex{ scaleSpace.getBaseNote(logicalIndex) };
        const auto scaleSpaceSize{ scaleSpace.storedSize() };

        auto repeatedSelection{ baseIndex };

        while (repeatedSelection < notesCount)
        {
            selectedNotes.push_back(repeatedSelection);
            repeatedSelection += scaleSpaceSize;
        }

        for (const auto& note : selectedNotes)
        {
            auto repeatingIndex{ baseIndex };

            while (repeatingIndex < notesCount)
            {
                selectionModel->select(currentModel()->index(repeatingIndex, note), selectionFlag);
                selectionModel->select(currentModel()->index(note, repeatingIndex), selectionFlag);

                repeatingIndex += scaleSpaceSize;
            }
        }
    }
    else if (QGuiApplication::keyboardModifiers() & Qt::ShiftModifier)
    {
        auto closestSelectedNote{ logicalIndex };

        while (closestSelectedNote != 0)
        {
            if (selectionModel->columnIntersectsSelection(closestSelectedNote))
                break;

            closestSelectedNote -= 1;
        }

        for (auto newIndex{ closestSelectedNote }; newIndex <= logicalIndex; ++ newIndex)
            for (const auto& selectedNote : selectedNotes)
            {
                selectionModel->select(currentModel()->index(newIndex, selectedNote), QItemSelectionModel::Select);
                selectionModel->select(currentModel()->index(selectedNote, newIndex), QItemSelectionModel::Select);
            }

        selectionModel->select({
                                currentModel()->index(closestSelectedNote, closestSelectedNote),
                                currentModel()->index(logicalIndex, logicalIndex)
                               },
                                QItemSelectionModel::Select);
    }
    else
    {
        for (const auto& selectedNote : selectedNotes)
        {
            selectionModel->select(currentModel()->index(logicalIndex, selectedNote), selectionFlag);
            selectionModel->select(currentModel()->index(selectedNote, logicalIndex), selectionFlag);
        }
    }
}

void MainWindow::populateModels()
{
    ui->scaleSpaceTable->setUpdatesEnabled(false);

    const auto range{ ui->rangeSpinBox->value() };

    sizeModel->clear();
    sizeModel->setColumnCount(range);
    sizeModel->setRowCount(range);
    QSignalBlocker sizeBlocker(sizeModel);

    weightModel->clear();
    weightModel->setColumnCount(range);
    weightModel->setRowCount(range);
    QSignalBlocker weightBlocker(weightModel);

    for (auto noteFrom{ 0 }; noteFrom != range; ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != range; ++noteTo)
        {
            auto sizeItem{new QStandardItem(makeSizeItemText(noteTo, noteFrom))};
            auto weightItem{new QStandardItem(makeWeightItemText(noteTo, noteFrom, ui->weightFuncCombo->currentIndex()))};

            if (itemShouldBeAltColour(noteFrom, noteTo, scaleSpace.storedSize()))
            {
                sizeItem->setBackground(QBrush(sizeItem->background().color().darker(settings::darknessFactor)));
                weightItem->setBackground(QBrush(sizeItem->background().color().darker(settings::darknessFactor)));
            }

            sizeModel->setItem(noteFrom, noteTo, sizeItem);
            weightModel->setItem(noteFrom, noteTo, weightItem);
        }

    ui->scaleSpaceTable->setUpdatesEnabled(true);
}

std::vector<int> MainWindow::getSelectedNotes() const
{
    if (!selectionIsSymetric())
        return {};

    const auto selectionModel{ ui->scaleSpaceTable->selectionModel() };

    std::vector<int> selected;

    //should only return true if a row is selected and it's column is selected
    for (auto note{ 0 }; note != currentModel()->rowCount(); ++note)
        if (selectionModel->isSelected(currentModel()->index(note, note)))
            selected.push_back(note);

    return selected;
}

void MainWindow::initialiseSaveSubScaleButtons()
{
    connect(ui->saveAsButton,
            &QPushButton::clicked,
            this,
            &MainWindow::handleSaveSubScaleSpaceAs);

    connect(ui->saveButton,
            &QPushButton::clicked,
            this,
            &MainWindow::handleSaveSubScaleSpace);
}

void MainWindow::initialiseMakeCancelButtons()
{
    ui->cancelButton->setDisabled(true);

    connect(ui->makeButton,
            &QPushButton::clicked,
            this,
            &MainWindow::handleMakeClicked);

    connect(ui->cancelButton,
            &QPushButton::clicked,
            this,
            &MainWindow::handleCancelClicked);

    connect(ui->clearButton,
            &QPushButton::clicked,
            this,
            [this]()
            {
                currentTuning.clear();
                ui->temperamentBox->clear();
                ui->makeProgressBar->setValue(0);
            });

    connect(&watcher,
            &QFutureWatcher<std::vector<long double>>::finished,
            this,
            &MainWindow::makingTuningFinished);
}

void MainWindow::handleSaveSubScaleSpaceAs()
{
    const auto url{ QFileDialog::getSaveFileUrl(this,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Scale Spaces (*." + dbUtils::filetypeName + ")") };

    if (dbManager::createDatabase(url))
    {
        const auto newDatabase{ dbManager::openDatabase(url) };

        dbManager::openDatabase(url)->savePattern(
            scaleSpace.makeSubSizePattern(getSelectedNotes()));

        QFileInfo info{ url.toLocalFile() };

        const auto file{ QFileInfo(dbUtils::makeUrlString(info.baseName(), info.absolutePath())) };

        currentFileUrl = QUrl::fromLocalFile(file.filePath());

        emit currentUrlChanged(currentFileUrl);

        changeTable(file.baseName(), newDatabase);

        ui->scaleSpaceCombo->setCurrentIndex(ui->scaleSpaceCombo->findText(settings::customScaleSpaceName));
    }
}

void MainWindow::handleMakeClicked()
{
    if (future.isRunning())
        return;

    ui->makeButton->setDisabled(true);

    ui->cancelButton->setEnabled(true);

    ui->clearButton->setDisabled(true);

    ui->temperamentBox->clear();

    tuneScaleCancelRequested = false;

    const auto notes{ getSelectedNotes() };
    const auto cutoff{ getCutoffValue() };
    const auto isUniform{ ui->weightFuncCombo->currentText() == settings::uniformWeightFunctionName};

    ui->makeProgressBar->setValue(0);

    ui->temperamentBox->setPlainText("Making tuning...");

    future = QtConcurrent::run([notes, cutoff, isUniform, this]()
    {
        Scale scale{ makeSubIntervalsPattern(notes) };
        scale.setWeightCutoff(cutoff);
        scale.setUnweighted(isUniform);

        return scale.tuneScale(tuneScaleCancelRequested,
        [this](int progress)
        {
            QMetaObject::invokeMethod(
                this,
                [this, progress]
                {
                    ui->makeProgressBar->setValue(progress);
                },
                Qt::QueuedConnection);
        });
    });

    watcher.setFuture(future);
}

void MainWindow::handleCancelClicked()
{
    tuneScaleCancelRequested = true;

    ui->makeButton->setEnabled(true);

    ui->cancelButton->setDisabled(true);

    ui->clearButton->setEnabled(true);

    displayTuning();
}

void MainWindow::makingTuningFinished()
{
    if (tuneScaleCancelRequested)
        return;

    ui->makeButton->setEnabled(true);

    ui->cancelButton->setDisabled(true);

    ui->clearButton->setEnabled(true);

    currentTuning = future.result();

    displayTuning();
}

void MainWindow::handleSaveSubScaleSpace()
{
    if (!currentFileUrl.isValid())
    {
        handleSaveSubScaleSpaceAs();
        return;
    }

    dbManager::openDatabase(currentFileUrl)->savePattern(
        scaleSpace.makeSubSizePattern(getSelectedNotes()));
}

int makeAdjustedRange(const int& currentRange, const int& oldScaleSpaceSize, const int& newScaleSpaceSize)
{
    if (oldScaleSpaceSize == newScaleSpaceSize)
        return currentRange;

    const auto newRange{ static_cast<float>(currentRange) *
                         static_cast<float>(newScaleSpaceSize) /
                         static_cast<float>(oldScaleSpaceSize) };

    if (newScaleSpaceSize > oldScaleSpaceSize)
    {
        return std::floor(newRange);
    }
    else
    {
        return std::ceil(newRange);
    }
}

bool MainWindow::selectionIsSymetric() const
{
    const auto selectionModel{ ui->scaleSpaceTable->selectionModel() };

    for (auto noteFrom{ 0 }; noteFrom != currentModel()->rowCount(); ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != currentModel()->columnCount(); ++noteTo)
        {
            if (selectionModel->isSelected(currentModel()->index(noteFrom, noteTo)) !=
                selectionModel->isSelected(currentModel()->index(noteTo, noteFrom)))
                return false;
        }

    return true;
}

IntervalsPattern MainWindow::makeSubIntervalsPattern(std::vector<int> notes) const
{
    if (notes.empty())
    {
        notes.resize(ui->rangeSpinBox->value());
        std::iota(notes.begin(), notes.end(), 0);
    }

    const auto notesSize{ notes.size() };

    std::sort(notes.begin(), notes.end());

    IntervalsPattern subIntervalsPattern;
    subIntervalsPattern.reserve(notesSize);

    for (auto noteFromIdx{ 0 }; noteFromIdx != notesSize - 1; ++noteFromIdx)
    {
        subIntervalsPattern.push_back(std::vector<Interval>());
        subIntervalsPattern.back().reserve(notesSize - noteFromIdx - 1);

        for (auto noteToIdx{ noteFromIdx + 1 }; noteToIdx != notesSize; ++noteToIdx)
        {
            subIntervalsPattern.back().push_back(
                {scaleSpace.getIntervalSize(notes[noteToIdx], notes[noteFromIdx]),
                 makeWeightItemValue(notes[noteToIdx], notes[noteFromIdx],
                                     ui->weightFuncCombo->currentIndex())});
        }
    }

    return subIntervalsPattern;
}

void MainWindow::displayTuning()
{
    ui->temperamentBox->clear();

    if (currentTuning.empty())
        return;

    ui->temperamentBox->appendPlainText("Ratio as tuning:");
    for (const auto& note : currentTuning)
        ui->temperamentBox->appendPlainText(ldtqs(note, globals::longDoubleLimit, false));

    ui->temperamentBox->appendPlainText("\nCents as tuning:");
    for (const auto& note : currentTuning)
        ui->temperamentBox->appendPlainText(ldtqs(centsFromRatio(note), globals::longDoubleLimit, false));
}

QString MainWindow::makeTooltipText(const QModelIndex &index)
{
    return { QString("Interval: %1 --> %2").arg(index.row() + 1).arg(index.column() + 1) +
            "\nSize (d): " + ldtqs(scaleSpace.getIntervalSize(index.column(), index.row()),
                             settings::precisionMax,
                             false) +
            "\nSize (c): " + ldtqs(centsFromRatio(scaleSpace.getIntervalSize(index.column(), index.row())),
                             settings::precisionMax,
                             true) +
            "\nWeight: " + makeWeightItemText(index.column(), index.row(), ui->weightFuncCombo->currentIndex()) };
}

void MainWindow::handleAddNote(int noteToAdd)
{
    const auto initialSize{ scaleSpace.storedSize() };

    scaleSpace.addNote(noteToAdd);

    QSignalBlocker blocker(ui->rangeSpinBox);

    ui->rangeSpinBox->setValue(makeAdjustedRange(ui->rangeSpinBox->value(),
                                                 initialSize,
                                                 initialSize + 1));

    populateModels();
}

void MainWindow::handleDeleteNote(int noteToDelete)
{
    const auto initialSize{ scaleSpace.storedSize() };

    scaleSpace.removeNote(noteToDelete);

    QSignalBlocker blocker(ui->rangeSpinBox);

    ui->rangeSpinBox->setValue(makeAdjustedRange(ui->rangeSpinBox->value(),
                                                 initialSize,
                                                 initialSize - 1));

    populateModels();
}

void MainWindow::initialiseRangeSpinBox()
{
    ui->rangeSpinBox->setRange(2, settings::maxTableSize);

    ui->rangeSpinBox->setValue(scaleSpace.size());

    const auto* increaseShortcut{ new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Equal), this) };

    connect(increaseShortcut,
            &QShortcut::activated,
            this,
            [this]()
            {
                ui->rangeSpinBox->setValue(ui->rangeSpinBox->value() + 1);
            });

    const auto* decreaseShortcut{ new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Minus), this) };

    connect(decreaseShortcut,
            &QShortcut::activated,
            this,
            [this]()
            {
                ui->rangeSpinBox->setValue(ui->rangeSpinBox->value() - 1);
            });

    connect(ui->rangeSpinBox,
            &QSpinBox::valueChanged,
            this,
            &MainWindow::handleRangeChanged);
}

void MainWindow::handleRangeChanged(int range)
{
    const auto oldRange{ currentModel()->rowCount() };

    if (range < oldRange)
    {
        sizeModel->setRowCount(range);
        sizeModel->setColumnCount(range);

        weightModel->setRowCount(range);
        weightModel->setColumnCount(range);

        cashedPreCustomWeightTable.resize(range);
        for (auto& row : cashedPreCustomWeightTable)
            row.resize(range);

        ui->scaleSpaceTable->verticalHeader()->resizeSections(QHeaderView::Stretch);
        ui->scaleSpaceTable->horizontalHeader()->resizeSections(QHeaderView::Stretch);
    }
    else if (range > oldRange)
    {
        for (auto newNote{ oldRange }; newNote != range; ++newNote)
        {
            QList<QStandardItem*> newSizeColumn;
            newSizeColumn.reserve(sizeModel->rowCount());
            QList<QStandardItem*> newSizeRow{ newSizeColumn };

            QList<QStandardItem*> newWeightColumn;
            newWeightColumn.reserve(weightModel->rowCount());
            QList<QStandardItem*> newWeightRow{ newWeightColumn };

            for(auto noteFrom{ 0 }; noteFrom != newNote; ++noteFrom)
            {
                newSizeColumn.push_back(new QStandardItem(makeSizeItemText(newNote, noteFrom)));
                newSizeRow.push_back(new QStandardItem(makeSizeItemText(noteFrom, newNote)));

                newWeightColumn.push_back(new QStandardItem(makeWeightItemText(newNote, noteFrom, ui->weightFuncCombo->currentIndex())));
                newWeightRow.push_back(new QStandardItem(makeWeightItemText(noteFrom, newNote, ui->weightFuncCombo->currentIndex())));

                if (itemShouldBeAltColour(noteFrom, newNote, scaleSpace.storedSize()))
                {
                    newSizeColumn.last()->setBackground(QBrush(newSizeColumn.last()
                        ->background().color().darker(settings::darknessFactor)));
                    newSizeRow.last()->setBackground(QBrush(newSizeRow.last()
                        ->background().color().darker(settings::darknessFactor)));

                    newWeightColumn.last()->setBackground(QBrush(newWeightColumn.last()
                        ->background().color().darker(settings::darknessFactor)));
                    newWeightRow.last()->setBackground(QBrush(newWeightRow.last()
                        ->background().color().darker(settings::darknessFactor)));
                }
            }

            newSizeRow.push_back(new QStandardItem(makeSizeItemText(newNote, newNote)));
            newWeightRow.push_back(new QStandardItem(makeWeightItemText(newNote, newNote, ui->weightFuncCombo->currentIndex())));

            if (itemShouldBeAltColour(newNote, newNote, scaleSpace.storedSize()))
            {
                newSizeRow.last()->setBackground(QBrush(newSizeRow.last()
                    ->background().color().darker(settings::darknessFactor)));

                newWeightRow.last()->setBackground(QBrush(newWeightRow.last()
                    ->background().color().darker(settings::darknessFactor)));
            }

            sizeModel->appendColumn(newSizeColumn);
            sizeModel->appendRow(newSizeRow);

            weightModel->appendColumn(newWeightColumn);
            weightModel->appendRow(newWeightRow);
        }
    }
}

void MainWindow::handleClearSelection()
{
    ui->scaleSpaceTable->selectionModel()->clearSelection();
}

void MainWindow::initialiseDisplaySettings()
{
    displayModeGroup->addButton(ui->ratioRadioButton, static_cast<int>(DisplayMode::ratio));
    displayModeGroup->addButton(ui->centsRadioButton, static_cast<int>(DisplayMode::cents));

    displayModeGroup->setExclusive(true);

    ui->ratioRadioButton->setChecked(true);

    connect(displayModeGroup,
            &QButtonGroup::idClicked,
            this,
            [this](int id)
            {
                const auto newDisplayMode{ static_cast<DisplayMode>(id) };

                if (displayMode == newDisplayMode)
                    return;

                switch (newDisplayMode)
                {
                case DisplayMode::ratio:
                    displayMode = DisplayMode::ratio;
                    break;

                case DisplayMode::cents:
                    displayMode = DisplayMode::cents;
                    break;

                default:
                    break;
                }

                refreshModels();
            });
}

void MainWindow::initialiseSizeWeightRatioGroup()
{
    sizeWeightModeGroup->addButton(ui->sizeRadioButton, static_cast<int>(IntervalMode::size));
    sizeWeightModeGroup->addButton(ui->weightRadioButton, static_cast<int>(IntervalMode::weight));

    sizeWeightModeGroup->setExclusive(true);

    ui->sizeRadioButton->setChecked(true);

    connect(sizeWeightModeGroup,
            &QButtonGroup::idClicked,
            this,
            [this](int id)
            {
                setIntervalMode(static_cast<IntervalMode>(id));
            });
}

long double MainWindow::makeSizeItemValue(const int& noteTo, const int& noteFrom) const
{
    switch (displayMode)
    {
    case DisplayMode::ratio:
        return scaleSpace.getIntervalSize(noteTo, noteFrom);
        break;

    case DisplayMode::cents:
        return centsFromRatio(scaleSpace.getIntervalSize(noteTo, noteFrom));
        break;

    default:
        return 1;
        break;
    }
}

void MainWindow::refreshModelsItemsText(const int& noteTo, const int& noteFrom)
{
    refreshSizeItemText(noteTo, noteFrom);
    refreshWeightItemText(noteTo, noteFrom);
}

void MainWindow::refreshSizeItemText(const int &noteTo, const int &noteFrom)
{
    shouldNotProcessSizeChange = true;

    sizeModel->item(noteFrom, noteTo)->setText(
        makeSizeItemText(noteTo, noteFrom));

    shouldNotProcessSizeChange = false;
}

void MainWindow::refreshWeightItemText(const int &noteTo, const int &noteFrom)
{
    shouldNotProcessWeightChange = true;

    weightModel->item(noteFrom, noteTo)->setText(
        makeWeightItemText(noteTo, noteFrom, ui->weightFuncCombo->currentIndex()));

    shouldNotProcessWeightChange = false;
}

long double MainWindow::intervalSizeAsRatio(const long double& size) const
{
    switch (displayMode)
    {
    case DisplayMode::ratio:
        return size;
        break;

    case DisplayMode::cents:
        return ratioFromCents(size);
        break;

    default:
        return 1;
        break;
    }
}

QString MainWindow::makeSizeItemText(const int& noteTo, const int& noteFrom) const
{
    return ldtqs(makeSizeItemValue(noteTo, noteFrom),
                 ui->precisionSpinBox->value(),
                 static_cast<bool>(displayMode));
}

void MainWindow::initialisePrecisionSpinBox()
{
    ui->precisionSpinBox->setRange(0, settings::precisionMax);

    ui->precisionSpinBox->setValue(settings::precision);

    connect(ui->precisionSpinBox,
            &QSpinBox::valueChanged,
            this,
            &MainWindow::refreshModels);
}

void MainWindow::handleCurrentUrlChanged(QUrl newUrl)
{
    QFileInfo info{ newUrl.toLocalFile() };

    ui->saveButton->setEnabled(ui->saveAsButton->isEnabled() &&
                               info.absoluteDir().canonicalPath() != dbUtils::databaseDirectory);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent{ static_cast<QKeyEvent*>(event) };

        if (keyEvent->key() == Qt::Key_Tab)
        {
            swapModel();
            return true;
        }

        if (obj == ui->scaleSpaceTable)
        {
            auto idx{ ui->scaleSpaceTable->currentIndex() };

            if (idx.isValid() && keyEvent->matches(QKeySequence::Copy))
            {
                QGuiApplication::clipboard()->setText(
                    idx.data(Qt::DisplayRole).toString());

                return true;
            }
            if (idx.isValid() && keyEvent->matches(QKeySequence::Paste))
            {
                ui->scaleSpaceTable->model()->setData(
                    idx,
                    QGuiApplication::clipboard()->text(),
                    Qt::EditRole);

                return true;
            }
            if (keyEvent->matches(QKeySequence::SelectAll))
            {
                ui->scaleSpaceTable->setSelectionMode(QAbstractItemView::MultiSelection);
                ui->scaleSpaceTable->selectAll();
                ui->scaleSpaceTable->setSelectionMode(QAbstractItemView::SingleSelection);
                return true;
            }
        }
    }
    if (event->type() == QEvent::ToolTip && obj == ui->scaleSpaceTable->viewport())
    {

        const auto* helpEvent{ static_cast<QHelpEvent*>(event) };

        const auto index{ ui->scaleSpaceTable->indexAt(helpEvent->pos()) };

        if (index.isValid())
        {
            QToolTip::showText(
                helpEvent->globalPos(),
                makeTooltipText(index),
                ui->scaleSpaceTable);

            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

void MainWindow::changeScaleSpace(const QString& newScaleSpaceName, const QString& newScaleSpaceDirectory)
{

}

void MainWindow::changeTable(const QString& newName, const std::unique_ptr<dbCurrnet>& newDatabase)
{
    if (newDatabase.get() == nullptr)
        return;

    const auto oldSize{ scaleSpace.storedSize() };

    scaleSpace.setScaleSpace(newName, newDatabase->loadPattern());

    QSignalBlocker blocker(ui->rangeSpinBox);

    ui->rangeSpinBox->setValue(makeAdjustedRange(ui->rangeSpinBox->value(), oldSize, scaleSpace.storedSize()));

    populateModels();

    setWindowTitle(newName + " - " + globals::appName);
}

QStandardItemModel* MainWindow::currentModel() const
{
    switch (intervalMode)
    {
    case IntervalMode::size:
        return sizeModel;
        break;

    case IntervalMode::weight:
        return weightModel;
        break;

    default:
        return nullptr;
        break;
    }
}

extern bool inputIsValid(const QString& input)
{
    bool canBeDouble;
    input.toDouble(&canBeDouble);

    return !(!(canBeDouble || inputCanBeFraction(input)) ||
           input.isEmpty() ||
           input.isNull());
}

void MainWindow::swapModel()
{
    setIntervalMode(intervalMode == IntervalMode::size ? IntervalMode::weight
                                                       : IntervalMode::size);
}

long double MainWindow::makeWeightItemValue(const int& noteTo, const int& noteFrom, const int& functionIndex) const
{
    if (ui->weightFuncCombo->itemText(functionIndex) != settings::customWeightFuncName)
    {
        const auto midDialValue{ (static_cast<long double>(ui->attenuationSlider->maximum()) -
                                 static_cast<long double>(ui->attenuationSlider->minimum())) / 2.L };

        const auto exponent{ std::pow(settings::attenuationScaling,
                                     1 - (ui->attenuationSlider->value() / midDialValue)) };

        return std::pow((weightFunctions.at(functionIndex).second)(scaleSpace.getIntervalSize(noteTo, noteFrom)),
                 exponent);
    }
    else if (noteFrom < cashedPreCustomWeightTable.size() &&
             noteTo < cashedPreCustomWeightTable.size())
    {
        return cashedPreCustomWeightTable[noteFrom][noteTo];
    }

    return 1;
}

QString MainWindow::makeWeightItemText(const int& noteTo, const int& noteFrom, const int& functionIndex) const
{
    //qDebug() << "inside makeWeightItemText";
    return ldtqs(makeWeightItemValue(noteTo, noteFrom, functionIndex),
                 ui->precisionSpinBox->value(),
                 true);
}

const long double MainWindow::getCutoffValue()
{
    const auto value{ ui->cutoffDial->maximum() - ui->cutoffDial->value() };

    const auto coeficient{ 1.L / static_cast<long double>(ui->rangeSpinBox->value()) };

    const auto base{ static_cast<long double>(value) /
                     static_cast<long double>(ui->cutoffDial->maximum()) };

    const auto exponent{ settings::cutoffValueExponentNumerator /
                         (std::log(factorial(ui->rangeSpinBox->value() - 2)) + 1) };

    return coeficient * std::pow(base, exponent);
}

void MainWindow::weightFunctionChanged(const int& newFunctionIndex)
{
    if (ui->weightFuncCombo->currentText() != settings::customWeightFuncName)
    {
        ui->attenuationSlider->setEnabled(true);
        cashedPreCustomWeightTable.clear();
    }
    else
    {
        ui->attenuationSlider->setEnabled(false);

        cashedPreCustomWeightTable.resize(weightModel->rowCount());

        for (auto noteFrom{ 0 }; noteFrom != cashedPreCustomWeightTable.size(); ++noteFrom)
        {
            cashedPreCustomWeightTable[noteFrom].resize(weightModel->columnCount());

            for (auto noteTo{ 0 }; noteTo != cashedPreCustomWeightTable[noteFrom].size(); ++noteTo)
            {
                cashedPreCustomWeightTable[noteFrom][noteTo] = makeWeightItemValue(noteTo, noteFrom, idxOfPrevWeightFunc);
            }
        }
    }

    refreshWeightModel();

    idxOfPrevWeightFunc = ui->weightFuncCombo->currentIndex();
}

void MainWindow::setIntervalMode(const IntervalMode& mode)
{
    if (intervalMode == mode)
        return;

    intervalMode = mode;

    // Update UI without relying on signals
    QSignalBlocker buttonGroupBlocker(sizeWeightModeGroup);

    if (mode == IntervalMode::size)
    {
        ui->sizeRadioButton->setChecked(true);
    }
    else if (mode == IntervalMode::weight)
    {
        ui->weightRadioButton->setChecked(true);
    }

    buttonGroupBlocker.unblock();
    buttonGroupBlocker.dismiss();

    const auto selectedIndexes{ ui->scaleSpaceTable->selectionModel()->selectedIndexes() };

    ui->scaleSpaceTable->setModel(intervalMode == IntervalMode::size ? sizeModel
                                                                     : weightModel);

    for (const auto& index : selectedIndexes)
        ui->scaleSpaceTable->selectionModel()->select(index, QItemSelectionModel::Select);

    setSelectionModelBehaviour();
}

void MainWindow::setSelectionModelBehaviour()
{
    connect(ui->scaleSpaceTable->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [this]()
            {
                const auto subScaleSpaceIsSaveable{ selectionIsSymetric() &&
                                                    ui->scaleSpaceTable->selectionModel()->selectedIndexes().size() != 1 };

                ui->saveAsButton->setEnabled(subScaleSpaceIsSaveable);

                QFileInfo info{ currentFileUrl.toLocalFile() };
                ui->saveButton->setEnabled(subScaleSpaceIsSaveable &&
                                           info.absoluteDir().canonicalPath() != dbUtils::databaseDirectory);
            });
}
