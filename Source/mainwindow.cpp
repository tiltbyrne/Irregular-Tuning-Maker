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
#include <QScrollBar>
#include <QLineEdit>
#include <QApplication>
#include <QTimer>

#include "weights.h"
#include "settings.h"
#include "dbManager.h"
#include "utilities.h"
#include "customheaderview.h"
#include "scale.h"
#include "scalespacedelegate.h"

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
    {
        if (tableDelegate()->getLastSelectedIndex())
            postModelResetSelect(tableDelegate()->getLastSelectedIndex().value(), std::nullopt);

        return;
    }

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
    ui->scaleSpaceTable->setItemDelegate(new ScaleSpaceDelegate(&scaleSpace, ui->scaleSpaceTable));
    ui->scaleSpaceTable->setMouseTracking(true);

    ui->scaleSpaceTable->viewport()->setMouseTracking(true);

    //ui->scaleSpaceTable->viewport()->installEventFilter(this);

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
}

void MainWindow::initialiseWeightFunctionsCombo()
{
    ui->weightFuncCombo->addItems(settings::weightFunctionNames);
    ui->weightFuncCombo->addItem(settings::customWeightFuncName);

    model->setWeightFunction(weightFunctions[ui->weightFuncCombo->currentIndex()].second);

    connect(ui->weightFuncCombo,
            &QComboBox::currentIndexChanged,
            this,
            &MainWindow::handleWeightFuncChanged);

    connect(model,
            &ScaleSpaceModel::weightModeArbitrary,
            this,
            [this]()
            {
                const auto blocker{ QSignalBlocker(ui->weightFuncCombo) };
                ui->weightFuncCombo->setCurrentText(settings::customWeightFuncName);
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

    connect(ui->cutoffDial,
            &QDial::actionTriggered,
            this,
            [this](int value)
            {
                handleCutoffChanged();
            });
}

void MainWindow::initialiseAttenuationSlider()
{
    ui->attenuationSlider->setValue(settings::attenuationMidpoint);

    connect(ui->attenuationSlider,
            &QDial::valueChanged,
            this,
            &MainWindow::handleAttenuationChanged);

    /*
    connect(ui->attenuationSlider,
            &QDial::sliderReleased,
            ui->attenuationSlider,
            [this]()
            {
                handleAttenuationChanged(ui->attenuationSlider->value());
            });
*/
}

void MainWindow::initialiseWindow()
{
    setWindowIcon(QIcon(":/images/icon.png"));
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

    updateSelectionBox();

    //updateSaveButtonStates();
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
    ui->temperamentBox->installEventFilter(this);
    ui->temperamentBox->setFixedWidth(ui->temperamentBox->width() + 24);

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

    tableDelegate()->setLastSelectedIndex(std::nullopt);

    const auto url{ QFileDialog::getSaveFileUrl(this,
                                                "Save File",
                                                QDir::currentPath(),
                                                "Scale Spaces (*." + dbUtils::filetypeName + ")") };

    if (!url.isValid() || url.isEmpty() || !dbManager::createDatabase(url))
        return;

    const auto newDatabase{ dbManager::openDatabase(url) };

    std::vector<int> notes{ notesToSave() };

    newDatabase->savePattern(scaleSpace.makeSubSizePattern(notes));

    QFileInfo info{ url.toLocalFile() };

    const auto file{ QFileInfo(dbUtils::makeUrlString(info.baseName(), info.absolutePath())) };

    currentFileUrl = QUrl::fromLocalFile(file.filePath());

    emit currentUrlChanged(currentFileUrl);

    changeTable(file.baseName(), newDatabase, false);

    ui->scaleSpaceCombo->setCurrentIndex(ui->scaleSpaceCombo->findText(settings::customScaleSpaceName));
}

void MainWindow::handleMakeClicked()
{
    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        startMaking();

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        startMaking();
    }
}

void MainWindow::startMaking()
{
    if (future.isRunning())
    {
        ui->cancelButton->click();
        QMetaObject::invokeMethod(this,
                                  [this]
                                  {
                                      this->startMaking();
                                  },
                                  Qt::QueuedConnection);

        return;
    }

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
                                   QMetaObject::invokeMethod(this,
                                                             [this, progress]
                                                             {
                                                                ui->makeProgressBar->setValue(progress);
                                                             },
                                                             Qt::QueuedConnection);
                               });
    });

    watcher.setFuture(future);
}

void MainWindow::cancelMaking()
{
    if (!future.isRunning())
        return;

    tuneScaleCancelRequested = true;

    ui->cancelButton->setDisabled(true);

    ui->clearButton->setEnabled(true);

    ui->temperamentBox->clear();

    //ui->makeProgressBar->setValue(0);
}

void MainWindow::clearTemperamentBox()
{
    if (future.isRunning())
        return;

    currentTuning.clear();
    ui->temperamentBox->clear();
    ui->makeProgressBar->setValue(0);
}

void MainWindow::handleCancelClicked()
{
    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        cancelMaking();

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        cancelMaking();
    }

    ui->makeProgressBar->setValue(0);
}

void MainWindow::makingTuningFinished()
{
    if (tuneScaleCancelRequested)
        return;

    tableDelegate()->setLastSelectedIndex(std::nullopt);

    ui->makeButton->setEnabled(true);

    ui->cancelButton->setDisabled(true);

    ui->clearButton->setEnabled(true);

    currentTuning = future.result();

    displayTuning();
}

void MainWindow::handlePrecisionChanged(const int &range)
{
    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        model->setPrecision(range);

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        model->setPrecision(range);
    }
}

void MainWindow::handleSaveSubScaleSpace()
{
    if (!currentFileUrl.isValid() || !ui->saveButton->isEnabled())
    {
        handleSaveSubScaleSpaceAs();
        return;
    }

    tableDelegate()->setLastSelectedIndex(std::nullopt);

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

    ui->temperamentBox->appendPlainText("Decimal tuning");

    for (const auto& note : currentTuning)
    {
        auto noteText{ ldtqs(note, settings::precisionMax, false) };

        if (!noteText.contains("."))
            noteText.append(".0");

        ui->temperamentBox->appendPlainText(noteText);
    }

    ui->temperamentBox->appendPlainText("\nCents tuning");

    for (const auto& note : currentTuning)
    {
        auto noteText{ ldtqs(centsFromRatio(note), settings::precisionMax, false) };

        if (!noteText.contains("."))
            noteText.append(".0");

        ui->temperamentBox->appendPlainText(noteText);
    }

    ui->temperamentBox->verticalScrollBar()->setValue(0);
    ui->temperamentBox->moveCursor(QTextCursor::Start);
}

QString MainWindow::makeTooltipText(const QModelIndex &index)
{
    const auto interval{ model->interval(index.row(), index.column()) };

    return { QString("Interval: [%1, %2]").arg(index.row() + 1).arg(index.column() + 1) +
            "\nSize (d): " + ldtqs(interval.getSize(), settings::precisionMax, false) +
            "\nSize (c): " + ldtqs(centsFromRatio(interval.getSize()), settings::precisionMax, true) +
            "\nWeight: " + ldtqs(interval.getWeight(), settings::precisionMax, true) };
}

void MainWindow::updateSaveButtonStates()
{
    QFileInfo info{ currentFileUrl.toLocalFile() };
    ui->saveButton->setEnabled(info.absoluteDir().canonicalPath() != dbUtils::databaseDirectory);
}

void MainWindow::handleClearClicked()
{
    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        clearTemperamentBox();

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        clearTemperamentBox();
    }
}

void MainWindow::postModelResetSelect(const QModelIndex &index, const std::optional<QModelIndex>& oldIndex)
{
    if (index.row() >= model->getRange() || index.column() >= model->getRange())
    {
        ui->scaleSpaceTable->selectionModel()->select(index, QItemSelectionModel::Deselect);
        tableDelegate()->setLastSelectedIndex(std::nullopt);
        return;
    }

    ui->scaleSpaceTable->setFocus();

    ui->scaleSpaceTable->setCurrentIndex(index);

    ui->scaleSpaceTable->scrollTo(index);

    ui->scaleSpaceTable->selectionModel()->select(index, QItemSelectionModel::Current);

    if (oldIndex.has_value())
        ui->scaleSpaceTable->update(oldIndex.value());
}

ScaleSpaceDelegate* MainWindow::tableDelegate() const
{
    return dynamic_cast<ScaleSpaceDelegate*>(ui->scaleSpaceTable->itemDelegate());
}

void MainWindow::updateSelectionBox()
{
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
}

std::vector<int> MainWindow::notesToSave() const
{
    auto notesToSave{ selectedNotes };

    if (notesToSave.empty())
    {
        const auto range{ ui->rangeSpinBox->value() };
        notesToSave.reserve(range);

        for (auto note{ 0 }; note != range; ++note)
            notesToSave.push_back(note);
    }

    return notesToSave;
}

void MainWindow::resetSelection()
{
    tableDelegate()->setLastSelectedIndex(std::nullopt);

    ui->scaleSpaceTable->selectionModel()->clearSelection();
    ui->scaleSpaceTable->setCurrentIndex({});
}

void MainWindow::handleAddNote(int noteToAdd)
{
    const auto initialSize{ scaleSpace.storedSize() };

    scaleSpace.addNote(noteToAdd);

    ui->rangeSpinBox->setValue(makeAdjustedRange(ui->rangeSpinBox->value(),
                                                 initialSize,
                                                 initialSize + 1));

    model->reset();

    auto baseNoteToAdd{ scaleSpace.getBaseNote(noteToAdd) };

    if (const auto selection{ tableDelegate()->getLastSelectedIndex()})
        postModelResetSelect(model->index(postAddNoteShift(baseNoteToAdd, selection.value().row(), initialSize),
                                          postAddNoteShift(baseNoteToAdd, selection.value().column(), initialSize)),
                             selection.value());

    for (auto& note : selectedNotes)
        note = postAddNoteShift(baseNoteToAdd, note, initialSize);

    updateSelectionBox();
}

void MainWindow::handleDeleteNote(int noteToDelete)
{
    const auto initialSize{ scaleSpace.storedSize() };

    auto baseNoteToDelete{ scaleSpace.getBaseNote(noteToDelete) };

    scaleSpace.removeNote(noteToDelete);

    ui->rangeSpinBox->setValue(makeAdjustedRange(ui->rangeSpinBox->value(),
                                                 initialSize,
                                                 initialSize - 1));

    model->reset();

    const auto isDeleted{ [&baseNoteToDelete, &initialSize](const int& note)
                         {
                             return ((note - baseNoteToDelete) % initialSize) == 0;
                         } };

    if (const auto selection{ tableDelegate()->getLastSelectedIndex()})
    {
        if (!isDeleted(selection->row()) && !isDeleted(selection->column()))
            postModelResetSelect(model->index(postRemoveNoteShift(baseNoteToDelete, selection.value().row(), initialSize),
                                              postRemoveNoteShift(baseNoteToDelete, selection.value().column(), initialSize)),
                                 selection.value());

        else
            tableDelegate()->setLastSelectedIndex(std::nullopt);
    }

    std::vector<int> indeciesToDelete;
    for (auto index{ 0 }; index != selectedNotes.size(); ++index)
    {
        if (!isDeleted(selectedNotes[index]))
            selectedNotes[index] = postRemoveNoteShift(baseNoteToDelete,
                                                       selectedNotes[index],
                                                       initialSize);
        else
            indeciesToDelete.push_back(index - indeciesToDelete.size());
    }

    for (const auto& index : indeciesToDelete)
        selectedNotes.erase(selectedNotes.begin() + index);

    updateSelectionBox();
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
    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        model->setRange(range);

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        model->setRange(range);
    }

    if (!selectedNotes.empty())
    {
        int newSize{ 0 };

        for (const auto& note : selectedNotes)
        {
            if (note >= range)
                break;

            ++newSize;
        }

        selectedNotes.resize(newSize);

        updateSelectionBox();
    }
}

void MainWindow::handleAttenuationChanged(int attenuation)
{
    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        model->setAttenuation(attenuation);

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        model->setAttenuation(attenuation);
    }
}

void MainWindow::handleCutoffChanged()
{
    if (!ui->scaleSpaceTable->selectionModel()->hasSelection())
        return;

    const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

    const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

    postModelResetSelect(selection, oldSelection);
}

void MainWindow::handleWeightFuncChanged(const int &index)
{
    auto setFunc{ [this](int index)
        {
            if (ui->weightFuncCombo->itemText(index) == settings::customWeightFuncName)
            {
                const QSignalBlocker blocker{ model };
                model->setWeightMode(WeightMode::Arbitrary);
            }
            else
                model->setWeightFunction(weightFunctions[index].second);

            //ui->attenuationSlider->setValue(settings::attenuationMidpoint);
        } };

    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        setFunc(index);

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        setFunc(index);
    }
}

void MainWindow::handleChangeDisplay(const DisplayMode &mode)
{
    displayModeGroup->button(static_cast<int>(mode))->setChecked(true);

    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        model->setDisplayMode(mode);

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        model->setDisplayMode(mode);
    }
}

void MainWindow::handleChangeSizeWeight(const IntervalMode &mode)
{
    sizeWeightModeGroup->button(static_cast<int>(mode))->setChecked(true);

    if (ui->scaleSpaceTable->selectionModel()->hasSelection())
    {
        const auto selection{ ui->scaleSpaceTable->selectionModel()->selectedIndexes()[0] };

        const auto oldSelection{ tableDelegate()->getLastSelectedIndex() };

        model->setIntervalMode(mode);

        postModelResetSelect(selection, oldSelection);
    }
    else
    {
        model->setIntervalMode(mode);
    }
}

void MainWindow::handleClearSelection()
{
    selectedNotes.clear();

    ui->selectionBox->clear();
}

void MainWindow::initialiseDisplaySettings()
{
    displayModeGroup->addButton(ui->ratioRadioButton, static_cast<int>(DisplayMode::ratio));
    displayModeGroup->addButton(ui->centsRadioButton, static_cast<int>(DisplayMode::cents));

    displayModeGroup->setExclusive(true);

    ui->ratioRadioButton->setChecked(true);

    connect(displayModeGroup,
            &QButtonGroup::idReleased,
            this,
            [this](int id)
            {
                handleChangeDisplay(static_cast<DisplayMode>(id));
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
            &QButtonGroup::idReleased,
            this,
            [this](int id)
            {
                handleChangeSizeWeight(static_cast<IntervalMode>(id));
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
    ui->precisionSpinBox->setRange(1, settings::precisionMax);

    ui->precisionSpinBox->setValue(settings::precision);
    model->setPrecision(settings::precision);

    connect(ui->precisionSpinBox,
            &QSpinBox::valueChanged,
            this,
            &MainWindow::handlePrecisionChanged);
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
    auto *keyEvent{ static_cast<QKeyEvent*>(event) };

    if (event->type() == QEvent::KeyPress)
    {
        if (obj == ui->scaleSpaceTable)
        {
            auto idx{ ui->scaleSpaceTable->currentIndex() };

            if (!idx.isValid())
                return QObject::eventFilter(obj, event);

            if (keyEvent->matches(QKeySequence::Copy))
            {
                QGuiApplication::clipboard()->setText(ldtqs(model->currentValue(idx.row(), idx.column())));
                return true;
            }
            if (keyEvent->matches(QKeySequence::Paste))
            {
                ui->scaleSpaceTable->model()->setData(idx, QGuiApplication::clipboard()->text(), Qt::EditRole);
                return true;
            }
            if (keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Backspace)
            {
                ui->scaleSpaceTable->model()->setData(idx, model->defaultText(), Qt::EditRole);
                return true;
            }
            if (keyEvent->key() == Qt::Key_Left &&
                keyEvent->modifiers() == Qt::ShiftModifier &&
                tableDelegate()->getLastSelectedIndex() != std::nullopt)
            {
                if (idx.column() - scaleSpace.storedSize() >= 0)
                    postModelResetSelect(model->index(idx.row(), idx.column() - scaleSpace.storedSize()), idx);

                return true;
            }
            if (keyEvent->key() == Qt::Key_Right &&
                keyEvent->modifiers() == Qt::ShiftModifier &&
                tableDelegate()->getLastSelectedIndex() != std::nullopt)
            {
                if (idx.column() + scaleSpace.storedSize() < model->getRange())
                    postModelResetSelect(model->index(idx.row(), idx.column() + scaleSpace.storedSize()), idx);

                return true;
            }
            if (keyEvent->key() == Qt::Key_Up &&
                keyEvent->modifiers() == Qt::ShiftModifier &&
                tableDelegate()->getLastSelectedIndex() != std::nullopt)
            {
                if (idx.row() - scaleSpace.storedSize() >= 0)
                    postModelResetSelect(model->index(idx.row() - scaleSpace.storedSize(), idx.column()), idx);

                return true;
            }
            if (keyEvent->key() == Qt::Key_Down &&
                keyEvent->modifiers() == Qt::ShiftModifier &&
                tableDelegate()->getLastSelectedIndex() != std::nullopt)
            {
                if (idx.row() + scaleSpace.storedSize() < model->getRange())
                    postModelResetSelect(model->index(idx.row() + scaleSpace.storedSize(), idx.column()), idx);

                return true;
            }
        }

        if (keyEvent->key() == Qt::Key_Tab)
        {
            swapDisplayMode();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Space)
        {
            swapIntervalMode();
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
            keyEvent->modifiers() == Qt::ControlModifier)
        {
            ui->makeButton->click();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Delete &&
            keyEvent->modifiers() == Qt::ControlModifier)
        {
            ui->cancelButton->click();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Backspace &&
            keyEvent->modifiers() == Qt::ControlModifier)
        {
            ui->clearButton->click();
            return true;
        }
        if (keyEvent->key() == Qt::Key_Up &&
            keyEvent->modifiers() == Qt::ControlModifier)
        {
            ui->cutoffDial->setValue(ui->cutoffDial->value() + 1);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Down &&
            keyEvent->modifiers() == Qt::ControlModifier)
        {
            ui->cutoffDial->setValue(ui->cutoffDial->value() - 1);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Left &&
            keyEvent->modifiers() == Qt::ControlModifier)
        {
            ui->attenuationSlider->setValue(ui->attenuationSlider->value() - 1);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Right &&
            keyEvent->modifiers() == Qt::ControlModifier)
        {
            ui->attenuationSlider->setValue(ui->attenuationSlider->value() + 1);
            return true;
        }
        if (keyEvent->key() == Qt::Key_Escape &&
            (tableDelegate()->getLastSelectedIndex() != std::nullopt ||
             ui->scaleSpaceTable->selectionModel()->hasSelection()))
        {
            resetSelection();
            return true;
        }
        if (tableDelegate()->getLastSelectedIndex() == std::nullopt &&
            (keyEvent->key() == Qt::Key_Left ||keyEvent->key() == Qt::Key_Right ||
             keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down))
        {
            return true;
        }
    }

    if (event->type() == QEvent::ToolTip && obj == ui->scaleSpaceTable->viewport())
    {
        const auto* helpEvent{ static_cast<QHelpEvent*>(event) };

        const auto index{ ui->scaleSpaceTable->indexAt(helpEvent->pos()) };

        if (index.isValid())
            QToolTip::showText(helpEvent->globalPos(), makeTooltipText(index), ui->scaleSpaceTable);

        return true;
    }

    if (event->type() == QEvent::FocusIn && obj == ui->temperamentBox)
    {
        tableDelegate()->setLastSelectedIndex(std::nullopt);
        ui->scaleSpaceTable->selectionModel()->clearSelection();
        return true;
    }

    return QObject::eventFilter(obj, event);
}

void MainWindow::changeTable(const QString& newName, const std::unique_ptr<dbCurrnet>& newDatabase, const bool& shouldAdjustRange)
{
    if (newDatabase.get() == nullptr)
        return;

    resetSelection();

    selectedNotes.clear();
    ui->selectionBox->clear();

    const auto oldSize{ scaleSpace.storedSize() };

    scaleSpace.setScaleSpace(newName, newDatabase->loadPattern());

    if (shouldAdjustRange)
        ui->rangeSpinBox->setValue(makeAdjustedRange(model->getRange(), oldSize, scaleSpace.storedSize()));

    model->reset();

    setWindowTitle(newName + " - " + globals::appName);
}

void MainWindow::swapIntervalMode()
{
    handleChangeSizeWeight(model->getIntervalMode() == IntervalMode::size ? IntervalMode::weight
                                                                         : IntervalMode::size);
}

void MainWindow::swapDisplayMode()
{
    handleChangeDisplay(model->getDisplayMode() == DisplayMode::ratio ? DisplayMode::cents
                                                                       : DisplayMode::ratio);
}

long double MainWindow::getCutoffValue() const
{
    return std::pow(static_cast<long double>(ui->cutoffDial->maximum() - ui->cutoffDial->value()) /
                        static_cast<long double>(ui->cutoffDial->maximum()),
                    std::pow(static_cast<long double>((settings::maxTableSize + 1) - notesToSave().size()),
                             settings::cutoffValueTuner)) /
           notesToSave().size();
}

int postAddNoteShift(int baseNoteAdded, const int originalNote, const int& scaleSpaceSize)
{
    if (baseNoteAdded == 0)
        baseNoteAdded = scaleSpaceSize;

    auto shiftedNote{ originalNote };
    auto adjustedRow{ baseNoteAdded };

    while (adjustedRow <= originalNote)
    {
        adjustedRow += scaleSpaceSize;
        ++shiftedNote;
    }

    return shiftedNote;
}

int postRemoveNoteShift(int baseNoteRemoved, const int originalNote, const int &scaleSpaceSize)
{
    auto shiftedNote{ originalNote };
    auto adjustedRow{ baseNoteRemoved };

    while (adjustedRow <= originalNote)
    {
        adjustedRow += scaleSpaceSize;
        --shiftedNote;
    }

    return shiftedNote;
}

void initialisePalette()
{
    QApplication::setStyle("Fusion");

    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Highlight, settings::highlight);
    pal.setColor(QPalette::Accent, settings::highlight);

    qApp->setPalette(pal);
}
