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
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setScaleSpaceValue(const int& noteTo, const int& noteFrom, const long double& value);

    void refreshModelsItemsText(const int& noteTo, const int& noteFrom);
    void refreshSizeItemText(const int& noteTo, const int& noteFrom);
    void refreshWeightItemText(const int& noteTo, const int& noteFrom);

signals:
    void currentUrlChanged(QUrl newUrl);
    void modelSwapped();

protected slots:
    void handleAddNote(int noteToAdd);
    void handleDeleteNote(int noteToDelete);
    void handleCurrentUrlChanged(QUrl newUrl);

private slots:
    void handleScaleSpaceActivated(QString scaleSpaceName);
    //void handleSizeModelItemChanged(QStandardItem* item);
    //void handleWeightModelItemChanged(QStandardItem* item);
    void handleHeaderLeftClicked(int logicalIndex);
    void handleSaveSubScaleSpace();
    void handleSaveSubScaleSpaceAs();
    void handleMakeClicked();
    void startMaking();
    void cancelMaking();
    void clearTemperamentBox();

    void handleCancelClicked();
    void makingTuningFinished();

    void handlePrecisionChanged(const int& range);
    void handleRangeChanged(const int& range);
    void handleAttenuationChanged(int attenuation);
    void handleCutoffChanged();
    void handleWeightFuncChanged(const int& index);

    void handleChangeDisplay(const DisplayMode& mode);
    void handleChangeSizeWeight(const IntervalMode& mode);

private:
    Ui::MainWindow *ui;
    std::atomic_bool tuneScaleCancelRequested{false};
    QFuture<std::vector<long double>> future;
    QFutureWatcher<std::vector<long double>> watcher;
    QUrl currentFileUrl;
    std::vector<long double> currentTuning;

    ScaleSpace scaleSpace;
    //QStandardItemModel* sizeModel;
    //QStandardItemModel* weightModel;
    ScaleSpaceModel* model;
    //bool selectionIsSymetric{ false };
    std::vector<int> selectedNotes;

    QButtonGroup* displayModeGroup;
    QButtonGroup* sizeWeightModeGroup;

    int idxOfPrevWeightFunc{ 0 };
    //std::vector<std::vector<long double>> cachedPreCustomWeightTable;

    bool shouldNotProcessSizeChange{ false };
    bool shouldNotProcessWeightChange{ false };

    const std::array<std::pair<QString, SizeToWeightFunction>, settings::weightFunctionsSize> weightFunctions;

    bool eventFilter(QObject *obj, QEvent *event) override;
    void changeScaleSpace(const QString& newScaleSpaceName, const QString& newScaleSpaceDirectory);
    void changeTable(const QString& newName, const std::unique_ptr<dbCurrnet>& newDatabase, const bool& shouldAdjustRange = true);

    //QStandardItemModel* currentModel() const;
    void swapIntervalMode();
    void swapDisplayMode();

    void refreshModels();
    void refreshWeightModel();
    void handleClearSelection();
    void setSelectionModelBehaviour();

    void initialiseTable();
    //void populateModels();
    void initialiseWeightFunctionsCombo();
    void initialiseScaleSpacesCombo();
    void initialiseCutoffDial();
    void initialiseAttenuationSlider();
    void initialiseSaveSubScaleButtons();
    void initialiseMakeCancelButtons();
    void initialiseRangeSpinBox();
    void initialisePrecisionSpinBox();
    void initialiseSelectionBox();

    void initialiseDisplaySettings();
    void initialiseSizeWeightRatioGroup();
    void initialiseWindow();

    long double intervalSizeAsRatio(const long double& size) const;

    const long double getCutoffValue();

    IntervalsPattern makeSubIntervalsPattern(std::vector<int> notes) const;

    void displayTuning();

    QString makeTooltipText(const QModelIndex& index);

    void updateSaveButtonStates();

    void handleClearClicked();

    void postModelResetSelect(const QModelIndex& index, const std::optional<QModelIndex>& oldIndex);

    void setLastSelectedCell(const QModelIndex& index);

    ScaleSpaceDelegate* tableDelegate() const;

    void updateSelectionBox();
};

extern int makeAdjustedRange(const int& currentRange, const int& oldScaleSpaceSize, const int& newScaleSpaceSize);
extern bool inputIsValid(const QString& input);
extern int postAddNoteShift(int baseNoteAdded, const int originalNote, const int& scaleSpaceSize);
extern int postRemoveNoteShift(int baseNoteRemoved, const int originalNote, const int& scaleSpaceSize);

#endif // MAINWINDOW_H
