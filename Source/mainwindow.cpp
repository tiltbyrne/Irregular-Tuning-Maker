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
#include "scale.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , weightFunctions(populateWeightFunctionsMap())
    , scaleSpace(settings::scaleSpaceName, dbManager::openDatabase(settings::scaleSpaceName)->loadPattern(), this)//if w is constructed after dbManager initialises then I think it's impossible for this to return null
    , currentFileUrl(QUrl::fromLocalFile(dbUtils::makeUrlString(settings::scaleSpaceName, dbUtils::databaseDirectory)))
    , model(new ScaleSpaceModel(&scaleSpace, this))
    , displayModeGroup(new QButtonGroup(this))
    , sizeWeightModeGroup(new QButtonGroup(this))
{
    ui->setupUi(this);

    QApplication::instance()->installEventFilter(this);
    initialiseWindow();
    setWindowIcon(QIcon(":/icon.png"));

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
    initialiseSelectionBox();
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

    selectedNotes.clear();

    ui->scaleSpaceTable->selectionModel()->clearSelection();
    ui->selectionBox->clear();

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

/*
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

        //refreshModels();
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
                   modelRows{ model->rowCount() },
                   modelColumns{ model->columnCount() };

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

        if (row < cachedPreCustomWeightTable.size() &&
            column < cachedPreCustomWeightTable.size())
        {
            cachedPreCustomWeightTable[row][column] = value;
            cachedPreCustomWeightTable[column][row] = value;
        }
    }
}
 */

/*
void MainWindow::refreshModels()
{
    for (auto noteFrom{ 0 }; noteFrom != model->rowCount(); ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != model->columnCount(); ++noteTo)
            refreshModelsItemsText(noteTo, noteFrom);
}

void MainWindow::refreshWeightModel()
{
    for (auto noteFrom{ 0 }; noteFrom != model->rowCount(); ++noteFrom)
        for (auto noteTo{ 0 }; noteTo != model->columnCount(); ++noteTo)
            refreshWeightItemText(noteTo, noteFrom);
}
*/

void MainWindow::initialiseTable()
{
    //debug crap
    /*
    ui->scaleSpaceTable->setModel(nullptr);
    auto *m = new QStandardItemModel(500, 500, this);

    for (int r = 0; r < 500; ++r)
        for (int c = 0; c < 500; ++c)
            m->setItem(r, c, new QStandardItem("x"));

    ui->scaleSpaceTable->setModel(m);
*/
    ///
    ui->scaleSpaceTable->setModel(model);
    ui->scaleSpaceTable->installEventFilter(this);

    ui->scaleSpaceTable->viewport()->installEventFilter(this);

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

    connect(&scaleSpace,
            &ScaleSpace::sizeChanged,
            horizontalHeader,
            &CustomHeaderView::handleScaleSpaceSizeChanged);

    connect(&scaleSpace,
            &ScaleSpace::sizeChanged,
            horizontalHeader,
            &CustomHeaderView::handleScaleSpaceSizeChanged);

    connect(&scaleSpace,
            &ScaleSpace::spaceTooSmall,
            horizontalHeader,
            [horizontalHeader](auto isTooSmall)
            {
                horizontalHeader->enableAction("delNote", !isTooSmall);
            });

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

    connect(ui->scaleSpaceTable->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [this]()
            {
                selectedNotes.clear();
                updateSaveButtonStates();
            });
}

void MainWindow::initialiseWeightFunctionsCombo()
{
    ui->weightFuncCombo->addItems(settings::weightFunctionNames);
    ui->weightFuncCombo->addItem(settings::customWeightFuncName);

    model->setWeightFunction(weightFunctions[ui->weightFuncCombo->currentIndex()].second);

    connect(ui->weightFuncCombo,
            &QComboBox::currentIndexChanged,
            this,
            [this](auto index)
            {
                if (weightFunctions[index].first == settings::customWeightFuncName)
                    model->setWeightMode(WeightMode::Arbitrary);
                else
                    model->setWeightFunction(weightFunctions[index].second);
            });
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
    ui->attenuationSlider->setValue(settings::attenuationMidpoint);

    connect(ui->attenuationSlider,
            &QDial::valueChanged,
            this,
            [this](auto value)
            {
                model->setAttenuation(value);
            });
}

void MainWindow::initialiseWindow()
{
    setWindowTitle(settings::scaleSpaceName + " - " + globals::appName);
}

void MainWindow::handleHeaderLeftClicked(int logicalIndex)
{
    const auto informSelectedNotes{ [this](const int& index, const QItemSelectionModel::SelectionFlag& selectionFlag)
        {
            if (selectionFlag == QItemSelectionModel::Select)
            {
                const auto itr{ std::find(selectedNotes.begin(), selectedNotes.end(), index) };
                if (itr == selectedNotes.end())
                    selectedNotes.push_back(index);
            }
            else if (selectionFlag == QItemSelectionModel::Deselect)
            {
                const auto itr{ std::remove(selectedNotes.begin(), selectedNotes.end(), index) };
                if (itr != selectedNotes.end())
                    selectedNotes.erase(itr, selectedNotes.end());
            }
        }};

    auto selectionFlag{ std::find(selectedNotes.begin(), selectedNotes.end(), logicalIndex) != selectedNotes.end()
                           ? QItemSelectionModel::Deselect
                           : QItemSelectionModel::Select };

    const auto notesCount{ model->getRange() };

    auto notesToIterateOver{ selectedNotes };

    QItemSelection selections;

    if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        const auto baseIndex{ scaleSpace.getBaseNote(logicalIndex) };

        const auto scaleSpaceSize{ scaleSpace.storedSize() };

        auto repeatedSelection{ baseIndex };

        while (repeatedSelection < notesCount)
        {
            notesToIterateOver.push_back(repeatedSelection);
            repeatedSelection += scaleSpaceSize;
        }

        auto repeatingIndex{ baseIndex };

        while (repeatingIndex < notesCount)
        {

            informSelectedNotes(repeatingIndex, selectionFlag);

            repeatingIndex += scaleSpaceSize;
        }
    }
    else
    {
        notesToIterateOver.push_back(logicalIndex);

        informSelectedNotes(logicalIndex, selectionFlag);
    }

    std::sort(selectedNotes.begin(), selectedNotes.end());

    ui->selectionBox->clear();

    if (!selectedNotes.empty())
    {
        QStringList list;

        for (const auto& note : selectedNotes)
            list << QString::number(note + 1);

        QString output{ "Selection: " };
        output.append(list.join(", "));

        ui->selectionBox->appendPlainText(output);
    }

    /*

    QElapsedTimer timer;
    timer.start();

    auto selectionModel{ ui->scaleSpaceTable->selectionModel() };

    QSignalBlocker selectionBlocker(selectionModel);

    if (!selectionIsSymetric)
    {
        selectionModel->clearSelection();
        selectedNotes.clear();
    }

    auto selectionFlag{ std::find(selectedNotes.begin(), selectedNotes.end(), logicalIndex) != selectedNotes.end()
                            ? QItemSelectionModel::Deselect
                            : QItemSelectionModel::Select };

    const auto notesCount{ model->getRange() };

    auto notesToIterateOver{ selectedNotes };

    QItemSelection selections;

    ui->scaleSpaceTable->viewport()->setUpdatesEnabled(false);

    if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        const auto baseIndex{ scaleSpace.getBaseNote(logicalIndex) };

        const auto scaleSpaceSize{ scaleSpace.storedSize() };

        auto repeatedSelection{ baseIndex };

        while (repeatedSelection < notesCount)
        {
            notesToIterateOver.push_back(repeatedSelection);
            repeatedSelection += scaleSpaceSize;
        }

        auto repeatingIndex{ baseIndex };

        while (repeatingIndex < notesCount)
        {
            for (const auto& note : notesToIterateOver)
            {
                const auto index{ model->index(repeatingIndex, note) };
                selections.select(index, index);

                const auto invIndex{ model->index(note, repeatingIndex) };
                selections.select(invIndex, invIndex);
            }

            informSelectedNotes(repeatingIndex, selectionFlag);

            repeatingIndex += scaleSpaceSize;
        }
    }
    else
    {
        notesToIterateOver.push_back(logicalIndex);

        for (const auto& note : notesToIterateOver)
        {
            const auto index{ model->index(logicalIndex, note) };
            selections.select(index, index);

            const auto invIndex{ model->index(note, logicalIndex) };
            selections.select(invIndex, invIndex);
        }

        informSelectedNotes(logicalIndex, selectionFlag);
    }

    selectionModel->select(selections, selectionFlag);

    ui->scaleSpaceTable->viewport()->setUpdatesEnabled(true);
    ui->scaleSpaceTable->viewport()->update();

    selectionIsSymetric = true;
*/
    updateSaveButtonStates();
}

/*
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
*/

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
            &MainWindow::handleClearClicked);

    connect(&watcher,
            &QFutureWatcher<std::vector<long double>>::finished,
            this,
            &MainWindow::makingTuningFinished);
}

void MainWindow::handleSaveSubScaleSpaceAs()
{
    if (!ui->saveAsButton->isEnabled())
        return;

    const auto url{ QFileDialog::getSaveFileUrl(this,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Scale Spaces (*." + dbUtils::filetypeName + ")") };

    if (!url.isValid() || url.isEmpty())
        return;

    if (dbManager::createDatabase(url))
    {
        const auto newDatabase{ dbManager::openDatabase(url) };

        std::vector<int> notesToSave{ selectedNotes };

        if (notesToSave.empty())
        {
            const auto range{ ui->rangeSpinBox->value() };
            notesToSave.reserve(range);

            for (auto note{ 0 }; note != range; ++note)
                notesToSave.push_back(note);
        }

        newDatabase->savePattern(
            scaleSpace.makeSubSizePattern(notesToSave));

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
    if (future.isRunning() || !ui->makeButton->isEnabled())
        return;

    ui->makeButton->setDisabled(true);

    ui->cancelButton->setEnabled(true);

    ui->clearButton->setDisabled(true);

    ui->makeProgressBar->setValue(0);

    ui->temperamentBox->setPlainText("Making tuning...");

    tuneScaleCancelRequested = false;

    const auto notes{ selectedNotes };
    const auto cutoff{ getCutoffValue() };
    const auto isUniform{ ui->weightFuncCombo->currentText() == settings::uniformWeightFunctionName};

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
    if (!ui->cancelButton->isEnabled())
        return;

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
    if (!currentFileUrl.isValid() || !ui->saveButton->isEnabled())
    {
        handleSaveSubScaleSpaceAs();
        return;
    }

    std::vector<int> notesToSave{ selectedNotes };

    if (notesToSave.empty())
    {
        const auto range{ ui->rangeSpinBox->value() };
        notesToSave.reserve(range);

        for (auto note{ 0 }; note != range; ++note)
            notesToSave.push_back(note);
    }

    dbManager::openDatabase(currentFileUrl)->savePattern(
        scaleSpace.makeSubSizePattern(selectedNotes));
}

int makeAdjustedRange(const int& currentRange, const int& oldScaleSpaceSize, const int& newScaleSpaceSize)
{
    if (oldScaleSpaceSize == newScaleSpaceSize)
        return currentRange;

    if (currentRange % oldScaleSpaceSize == 1)
        return 1 + (currentRange / oldScaleSpaceSize) * newScaleSpaceSize;

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

//bool MainWindow::selectionIsSymetric() const
//{
//    auto selectionModel{ ui->scaleSpaceTable->selectionModel() };
//    auto model{ currentModel() };

//    const int rows{ model->rowCount() };

//    for (int noteFrom = 0; noteFrom < rows; ++noteFrom)
//        for (int noteTo = noteFrom + 1; noteTo < rows; ++noteTo)
//            if (selectionModel->isSelected(model->index(noteFrom, noteTo)) !=
//                selectionModel->isSelected(model->index(noteTo, noteFrom)))
//                return false;

//    return true;
//}

IntervalsPattern MainWindow::makeSubIntervalsPattern(std::vector<int> notes) const
{
    if (notes.empty())
    {
        notes.resize(model->getRange());
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
            subIntervalsPattern.back().push_back(model->interval(notes[noteFromIdx], notes[noteToIdx]));
    }

    return subIntervalsPattern;
}

void MainWindow::displayTuning()
{
    ui->temperamentBox->clear();

    if (currentTuning.empty())
        return;

    ui->temperamentBox->appendPlainText("Decimals as tuning:");
    for (const auto& note : currentTuning)
        ui->temperamentBox->appendPlainText(ldtqs(note, settings::precisionMax, false));

    ui->temperamentBox->appendPlainText("\nCents as tuning:");
    for (const auto& note : currentTuning)
        ui->temperamentBox->appendPlainText(ldtqs(centsFromRatio(note), settings::precisionMax, false));
}

QString MainWindow::makeTooltipText(const QModelIndex &index)
{
    const auto size{ scaleSpace.getIntervalSize(index.column(), index.row()) };

    return { QString("Interval: %1 --> %2").arg(index.row() + 1).arg(index.column() + 1) +
            "\nSize (d): " + ldtqs(size, settings::precisionMax, false) +
            "\nSize (c): " + ldtqs(centsFromRatio(size), settings::precisionMax, true) +
            "\nWeight: " + ldtqs(model->weightValue(index.column(), index.row()), settings::precisionMax, true) };
}

void MainWindow::updateSaveButtonStates()
{
    QElapsedTimer timer;
    timer.start();

    //const auto subScaleSpaceIsSaveable{ ui->scaleSpaceTable->selectionModel()->hasSelection() };

    //qDebug() << "Checking savability took" << timer.elapsed() << "ms";

    QFileInfo info{ currentFileUrl.toLocalFile() };
    ui->saveButton->setEnabled(info.absoluteDir().canonicalPath() != dbUtils::databaseDirectory);
}

void MainWindow::handleClearClicked()
{
    if (future.isRunning() || !ui->clearButton->isEnabled())
        return;

    currentTuning.clear();
    ui->temperamentBox->clear();
    ui->makeProgressBar->setValue(0);
}

void MainWindow::handleAddNote(int noteToAdd)
{
    const auto initialSize{ scaleSpace.storedSize() };

    scaleSpace.addNote(noteToAdd);

    ui->rangeSpinBox->setValue(makeAdjustedRange(ui->rangeSpinBox->value(),
                                                 initialSize,
                                                 initialSize + 1));

    model->recomputeCache();

    //populateModels();
}

void MainWindow::handleDeleteNote(int noteToDelete)
{
    const auto initialSize{ scaleSpace.storedSize() };

    scaleSpace.removeNote(noteToDelete);

    //QSignalBlocker blocker(ui->rangeSpinBox);

    ui->rangeSpinBox->setValue(makeAdjustedRange(ui->rangeSpinBox->value(),
                                                 initialSize,
                                                 initialSize - 1));

    model->recomputeCache();
    //populateModels();
}

void MainWindow::initialiseRangeSpinBox()
{
    ui->rangeSpinBox->setRange(2, settings::maxTableSize);
    ui->rangeSpinBox->setValue(scaleSpace.size());

    const auto* increaseShortcut{ new QShortcut(QKeyCombination(Qt::ControlModifier, Qt::Key_Equal), this) };

    connect(increaseShortcut,
            &QShortcut::activated,
            this,
            [this]()
            {
                ui->rangeSpinBox->setValue(ui->rangeSpinBox->value() + 1);
            });

    const auto* decreaseShortcut{ new QShortcut(QKeyCombination(Qt::ControlModifier, Qt::Key_Minus), this) };

    connect(decreaseShortcut,
            &QShortcut::activated,
            this,
            [this]()
            {
                ui->rangeSpinBox->setValue(ui->rangeSpinBox->value() - 1);
            });

    const auto* increaseBySizeShortcut{ new QShortcut(QKeyCombination(Qt::ShiftModifier, Qt::Key_Equal), this) };

    connect(increaseBySizeShortcut,
            &QShortcut::activated,
            this,
            [this]()
            {
                qDebug() << model->getRange() << settings::maxTableSize - scaleSpace.storedSize();

                if (model->getRange() <= settings::maxTableSize - scaleSpace.storedSize())
                    ui->rangeSpinBox->setValue(ui->rangeSpinBox->value() + scaleSpace.storedSize());
            });

    const auto* decreaseBySizeShortcut{ new QShortcut(QKeyCombination(Qt::ShiftModifier, Qt::Key_Minus), this) };

    connect(decreaseBySizeShortcut,
            &QShortcut::activated,
            this,
            [this]()
            {
                if (model->getRange() > scaleSpace.size())
                    ui->rangeSpinBox->setValue(ui->rangeSpinBox->value() - scaleSpace.storedSize());
            });

    connect(ui->rangeSpinBox,
            &QSpinBox::valueChanged,
            this,
            &MainWindow::handleRangeChanged);
}

void MainWindow::handleRangeChanged(const int& range)
{
    model->setRange(range);
}

void MainWindow::handleClearSelection()
{
    ui->scaleSpaceTable->selectionModel()->clearSelection();

    selectedNotes = {};

    ui->selectionBox->clear();
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
                model->setDisplayMode(static_cast<DisplayMode>(id));
            });
}

void MainWindow::initialiseSizeWeightRatioGroup()
{
    sizeWeightModeGroup->addButton(ui->sizeRadioButton, static_cast<int>(IntervalMode::size));
    sizeWeightModeGroup->addButton(ui->weightRadioButton, static_cast<int>(IntervalMode::weight));

    sizeWeightModeGroup->setExclusive(true);

    ui->sizeRadioButton->setChecked(true);
    model->setIntervalMode(IntervalMode::size);

    connect(sizeWeightModeGroup,
            &QButtonGroup::idClicked,
            this,
            [this](int id)
            {
                model->setIntervalMode(static_cast<IntervalMode>(id));
            });
}

/*
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
*/

void MainWindow::initialisePrecisionSpinBox()
{
    ui->precisionSpinBox->setRange(0, settings::precisionMax);

    ui->precisionSpinBox->setValue(settings::precision);
    model->setPrecision(settings::precision);

    connect(ui->precisionSpinBox,
            &QSpinBox::valueChanged,
            this,
            [this](int value)
            {
                model->setPrecision(value);
            });
}

void MainWindow::initialiseSelectionBox()
{
    const auto& selectionBox{ ui->selectionBox };
    selectionBox->setFixedHeight(selectionBox->fontMetrics().height() +
                                 2 * selectionBox->contentsMargins().top() + 8);

    selectionBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void MainWindow::handleCurrentUrlChanged(QUrl newUrl)
{
    QFileInfo info{ newUrl.toLocalFile() };

    ui->saveButton->setEnabled(info.absoluteDir().canonicalPath() != dbUtils::databaseDirectory);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {


        auto *keyEvent{ static_cast<QKeyEvent*>(event) };

        if (keyEvent->key() == Qt::Key_Tab)
        {
            swapIntervalMode();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Space)
        {
            swapDisplayMode();
            return true;
        }
        if (keyEvent->matches(QKeySequence::Save))
        {
            handleSaveSubScaleSpace();
            return true;
        }
        if (keyEvent->matches(QKeySequence::SaveAs))
        {
            handleSaveSubScaleSpaceAs();
            return true;
        }
        if ((keyEvent->key() == Qt::Key_Return ||
             keyEvent->key() == Qt::Key_Enter) &&
            (keyEvent->modifiers() & Qt::ControlModifier))
        {
            handleMakeClicked();
            return true;
        }
        if (keyEvent->matches(QKeySequence::Cancel))
        {
            handleCancelClicked();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Delete && keyEvent->modifiers() == Qt::ControlModifier)
        {
            handleClearClicked();
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
        }
    }
    if (event->type() == QEvent::ToolTip && obj == ui->scaleSpaceTable->viewport())
    {
        const auto* helpEvent{ static_cast<QHelpEvent*>(event) };

        const auto index{ ui->scaleSpaceTable->indexAt(helpEvent->pos()) };

        if (index.isValid())
        {
            QToolTip::showText(helpEvent->globalPos(), makeTooltipText(index), ui->scaleSpaceTable);

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

    //QSignalBlocker blocker(ui->rangeSpinBox);

    ui->rangeSpinBox->setValue(makeAdjustedRange(model->getRange(), oldSize, scaleSpace.storedSize()));

    model->recomputeCache();

    setWindowTitle(newName + " - " + globals::appName);
}

/*
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
*/

/*
extern bool inputIsValid(const QString& input)
{
    bool canBeDouble;
    input.toDouble(&canBeDouble);

    return !(!(canBeDouble || inputCanBeFraction(input)) ||
           input.isEmpty() ||
           input.isNull());
}
*/

void MainWindow::swapIntervalMode()
{
    const auto newIntervalMode{ model->getIntervalMode() == IntervalMode::size ? IntervalMode::weight
                                                                               : IntervalMode::size};

    model->setIntervalMode(newIntervalMode);

    sizeWeightModeGroup->button(static_cast<int>(newIntervalMode))->setChecked(true);
}

void MainWindow::swapDisplayMode()
{
    const auto newDisplayMode{ model->getDisplayMode() == DisplayMode::ratio ? DisplayMode::cents
                                                                              : DisplayMode::ratio};

    model->setDisplayMode(newDisplayMode);

    displayModeGroup->button(static_cast<int>(newDisplayMode))->setChecked(true);
}

/*
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
    else if (noteFrom < cachedPreCustomWeightTable.size() &&
             noteTo < cachedPreCustomWeightTable.size())
    {
        return cachedPreCustomWeightTable[noteFrom][noteTo];
    }

    return 1;
}
*/

/*
QString MainWindow::makeWeightItemText(const int& noteTo, const int& noteFrom, const int& functionIndex) const
{
    //qDebug() << "inside makeWeightItemText";
    return ldtqs(makeWeightItemValue(noteTo, noteFrom, functionIndex),
                 ui->precisionSpinBox->value(),
                 true);
}
*/

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

/*
void MainWindow::weightFunctionChanged(const int& newFunctionIndex)
{
    if (ui->weightFuncCombo->currentText() != settings::customWeightFuncName)
    {
        ui->attenuationSlider->setEnabled(true);
        cachedPreCustomWeightTable.clear();
    }
    else
    {
        ui->attenuationSlider->setEnabled(false);

        cachedPreCustomWeightTable.resize(ui->rangeSpinBox->value());

        for (auto noteFrom{ 0 }; noteFrom != cachedPreCustomWeightTable.size(); ++noteFrom)
        {
            cachedPreCustomWeightTable[noteFrom].resize(weightModel->columnCount());

            for (auto noteTo{ 0 }; noteTo != cachedPreCustomWeightTable[noteFrom].size(); ++noteTo)
            {
                cachedPreCustomWeightTable[noteFrom][noteTo] = makeWeightItemValue(noteTo, noteFrom, idxOfPrevWeightFunc);
            }
        }
    }

    refreshWeightModel();

    idxOfPrevWeightFunc = ui->weightFuncCombo->currentIndex();
}
*/
