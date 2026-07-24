#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore>
#include <QTableView>
#include <QStandardItemModel>
#include <QButtonGroup>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <atomic>

#include "scaleSpace.h"
#include "dbcurrnet.h"
#include "settings.h"
#include "interval.h"
#include "scalespacemodel.h"
#include "scalespacedelegate.h"

using IntervalsPattern = std::vector<std::vector<Interval>>;
using SizeToWeightFunction = std::function<long double(const long double&)>;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //almost everything in the function body is handled by initialisation functions
    MainWindow(QWidget *parent = nullptr);

    //must cancel the future and also wait for it to finish
    ~MainWindow();

signals:
    // emited whenever the url storing the current scale space changes to newUrl
    void currentUrlChanged(QUrl newUrl);

protected slots:
    // adds note at index noteToAdd to the scale space and informs selectedNotes, model, and range
    void handleAddNote(int noteToAdd, bool cameFromAddBefore);

    // deletes note at index noteToDelete to the scale space and informs selectedNotes, model, and range
    void handleDeleteNote(int noteToDelete);

    void handleFillSelection();

    // sets currentUrl to newUrl and informs saveButton
    void handleCurrentUrlChanged(QUrl newUrl);

private slots:
    //handles begining the process of changing to the scaleSpace that corresponds to itemName, or custom scale chosen
    void handleScaleSpaceActivated(QString itemName);

    //this just adds the note at logicalIndex to selectedNotes and resets selection if there is one
    void handleHeaderLeftClicked(int logicalIndex);

    //saves sub scale space and resets selection if there is one
    void handleSaveSubScaleSpace();

    //saves sub scale space as and resets selection if there is one
    void handleSaveSubScaleSpaceAs();

    //begins the process of making the tuning of the current sub scale and resets selection if there is one
    void handleMakeClicked();

    //begins the process of canceling tuning and resets selection if there is one
    void handleCancelClicked();

    //clears whatever scale is currently displayed in the display box and resets selection if there is one
    void handleClearClicked();

    //informs the model of a change in display precision and resets selection if there is one
    void handlePrecisionChanged(const int& range);

    //informs the model of a change in range and resets selection if there is one
    void handleRangeChanged(const int& range);

    //informs the model of a change in attenuation and resets selection if there is one
    void handleAttenuationChanged(int attenuation);

    //resets selection if there is one
    void handleCutoffChanged();

    //informs the model of a change in weight function and resets selection if there is one
    void handleWeightFuncChanged(const int& index);

    //informs the model of a change in display mode and resets selection if there is one
    void handleChangeDisplay(const DisplayMode& mode);

    //informs the model of a change in interval mode and resets selection if there is one
    void handleChangeSizeWeight(const IntervalMode& mode);

    //this is the only function which liases with the Scale class, begins the process of tuning the scale
    void startMaking();

    //cancels making the scale if that is running
    void cancelMaking();

    //informs buttons and tuning display box once tuning has finished
    void makingTuningFinished();

    //clears the temperament box
    void clearTemperamentBox();

private:
    //pointer to ui
    Ui::MainWindow *ui;

    //set bool to true when scale tuning is canceled, remember to reset to false once done
    std::atomic_bool tuneScaleCancelRequested{false};

    //the tuning which is concurrently being made
    QFuture<std::vector<long double>> tuning;

    //watches tuning
    QFutureWatcher<std::vector<long double>> watcher;

    //the url of the currently selected scale space file
    QUrl currentFileUrl;

    //the last tuning which was made
    std::vector<long double> currentTuning;

    //this object handles a lot of the logic of accessing notes in different octaves and inversions
    ScaleSpace scaleSpace;

    //the model for the central table of the application
    ScaleSpaceModel* model;

    //only these notes will be saved/have a tuning made of them if save/make buttons are pressed, if empty all notes will be saved/made
    std::vector<int> selectedNotes;

    //radio group for displayMode
    QButtonGroup* displayModeGroup;

    //radio group for intervalMode
    QButtonGroup* sizeWeightModeGroup;

    //stores all weight functions, for details see weights.cpp
    const std::array<std::pair<QString, SizeToWeightFunction>, settings::weightFunctionsSize> weightFunctions;

    //almost all hotkeys and the like are handled here
    bool eventFilter(QObject *obj, QEvent *event) override;

    //handles everything to do with changing the database (model, range etc.)
    void changeDatabase(const QString& newName, const std::unique_ptr<dbCurrnet>& newDatabase, const bool& shouldAdjustRange = true);

    //helper function changes intervalMode to whatever it isn't
    void swapIntervalMode();

    //helper function changes displayMode to whatever it isn't
    void swapDisplayMode();

    //helper function clears selection box and empties selectedNotes
    void handleClearSelection();

    //takes the valu from the dial turns it into a cutoff value for the Scale member object for tuning
    long double getCutoffValue() const;

    //makes and IntervalPattern contining all intervals from/to the notes in notes which is a subset of the scaleSpace member object
    IntervalsPattern makeSubIntervalsPattern(std::vector<int> notes) const;

    //puts the current tuning into the display box
    void displayTuning();

    //displays the details (interval to/from, size, weight etc.) of the note at index
    QString makeTooltipText(const QModelIndex& index);

    //resets the selection onto index
    void postModelResetSelect(const QModelIndex& index, const std::optional<QModelIndex>& oldIndex);

    //helper function returns the delegate for the scaleSpaceTable
    ScaleSpaceDelegate* tableDelegate() const;

    //fills selectionBox with the notes in selectedNotes
    void updateSelectionBox();

    //if selectedNotes is empty this returns all notes [0, model->getRange()]
    std::vector<int> notesToSave() const;

    //clears the selection on the table
    void resetSelection();

    //specific intialisation helper function-------------------------------------------------------
    void initialiseTable();
    void initialiseWeightFunctionsCombo();
    void initialiseScaleSpacesCombo();
    void initialiseCutoffDial();
    void initialiseAttenuationSlider();
    void initialiseSaveSubScaleButtons();
    void initialiseMakeCancelButtons();
    void initialiseRangeSpinBox();
    void initialisePrecisionSpinBox();
    void initialiseSelectionBox();
    void initialiseSizeWeightRatioGroup();
    void initialiseDisplaySettings();
    //---------------------------------------------------------------------------------------------

    //general intialisation helper functions
    void initialiseWindow();
};

//addapts the current range to new range such that theres the same proportion of each scale space as there was
extern int makeAdjustedRange(const int& currentRange, const int& oldScaleSpaceSize, const int& newScaleSpaceSize);

//fidly helper function to which returns the new index of a note when its added
extern int postAddNoteShift(int baseNoteAdded, const int originalNote, const int& scaleSpaceSize);

//fidly helper function to which returns the new index of a note when its removed
extern int postRemoveNoteShift(int baseNoteRemoved, const int originalNote, const int& scaleSpaceSize);

//sets the global palette for the app
extern void initialisePalette();

#endif // MAINWINDOW_H
