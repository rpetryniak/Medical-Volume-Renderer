#include "vtkKWMyWindow.h"

//KWWidget headers:
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkKWRenderWidget.h"
#include "vtkKWSimpleAnimationWidget.h"
#include "vtkKWFrameWithScrollbar.h"
#include "vtkKWFrameWithLabel.h"
#include "vtkKWVolumePropertyWidget.h"
#include "vtkKWColorPresetMenu.h"
#include "vtkKWWidgetsPaths.h"
#include "vtkKWMenu.h"
#include "vtkKWLoadSaveDialog.h"

//VTK headers:
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkProperty.h"
#include "vtkStructuredPointsReader.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkVolumeRayCastCompositeFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkToolkits.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkKWEvent.h"

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMyWindow );
vtkCxxRevisionMacro( vtkKWMyWindow, "$Revision: 1.4 $");

//----------------------------------------------------------------------------
vtkKWMyWindow::vtkKWMyWindow()
{
  vtkReader  = vtkStructuredPointsReader::New();
}

//----------------------------------------------------------------------------
vtkKWMyWindow::~vtkKWMyWindow()
{
  if (this->renderWidget)     this->renderWidget->Delete();
}

void vtkKWMyWindow::RefreshRenderer(vtkObject *caller, unsigned long eid, void *clientData, void *callData)
{
  vtkKWMyWindow *self = reinterpret_cast<vtkKWMyWindow *>(clientData);
  
  if (self)
    self->renderWidget->Render();
}

//----------------------------------------------------------------------------
void vtkKWMyWindow::CreateWidget()
{
  // Check if already created
  if (this->IsCreated())
  {
    vtkErrorMacro("class already created");
    return;
  }
  // Call the superclass to create the whole widget
  this->Superclass::CreateWidget();
  vtkKWApplication *app = this->GetApplication();

  //Prepare menus:
  int index;
  vtkKWMenu *openMenu = this->GetFileMenu() ;

  index = openMenu->InsertCommand(openMenu->GetNumberOfItems()-2,"Open VTK test data", this, "openVtkFileTestData");
    openMenu->SetBindingForItemAccelerator(index, openMenu->GetParentTopLevel());
    openMenu->SetItemHelpString(index, "Open VTK test data (SciRUN tooth file).");
  index = openMenu->InsertCommand(openMenu->GetNumberOfItems()-2,"Open &Vtk File", this, "openVtkFileDialog");
    openMenu->SetBindingForItemAccelerator(index, openMenu->GetParentTopLevel());
    openMenu->SetItemHelpString(index, "Open VTK Structured Points file format.");
  openMenu->InsertSeparator (openMenu->GetNumberOfItems()-2) ;

  // Add a render widget, attach it to the view frame, and pack
  renderWidget = vtkKWRenderWidget::New();
    renderWidget->SetParent(this->GetViewFrame());
    renderWidget->Create();
    renderWidget->SetRendererBackgroundColor(0.5, 0.6, 0.8);
    renderWidget->SetRendererGradientBackground(4);

    app->Script("pack %s -expand y -fill both -anchor c -expand y", renderWidget->GetWidgetName());

  cast = vtkImageCast::New();
    cast->SetOutputScalarTypeToUnsignedShort();
    cast->ClampOverflowOn();

  //create a scrolled frame for Volume Rendering Properties
  vtkKWFrameWithScrollbar *vpw_frame = vtkKWFrameWithScrollbar::New();
    vpw_frame->SetParent(this->GetMainPanelFrame());
    vpw_frame->Create();

    app->Script("pack %s -side top -fill both -expand y", vpw_frame->GetWidgetName());

  // Create a color preset selector
  cpsel = vtkKWColorPresetMenu::New();
    cpsel->SetParent(vpw_frame->GetFrame());
    cpsel->Create();
    cpsel->SetPresetSelectedCommand(NULL, "update_editor");
    cpsel->SetPresetSelectedCommand(this, "colorPresetSelectedCommand");
    cpsel->SetBalloonHelpString(
      "A set of predefined color presets. Select one of them to apply the "
      "preset to a color transfer function (vtkColorTransferFunction)");

    app->Script("pack %s -side top -anchor nw -expand n -padx 2 -pady 2", cpsel->GetWidgetName());

  // Create a volume property widget
  volumePropertyWidget = vtkKWVolumePropertyWidget::New();
    volumePropertyWidget->SetParent(vpw_frame->GetFrame());
    volumePropertyWidget->Create();

    app->Script("pack %s -side top -anchor nw -expand y -padx 2 -pady 2", volumePropertyWidget->GetWidgetName());
    
  // Create a simple animation widget
  vtkKWFrameWithLabel *animation_frame = vtkKWFrameWithLabel::New();
    animation_frame->SetParent(vpw_frame->GetFrame());
    animation_frame->Create();
    animation_frame->SetLabelText("Movie Creator");

    app->Script("pack %s -side top -anchor nw -expand n -fill x -pady 2", animation_frame->GetWidgetName());

  vtkKWSimpleAnimationWidget *animation_widget = vtkKWSimpleAnimationWidget::New();
    animation_widget->SetParent(animation_frame->GetFrame());
    animation_widget->Create();
    animation_widget->SetRenderWidget(renderWidget);
    animation_widget->SetAnimationTypeToCamera();

    app->Script("pack %s -side top -anchor nw -expand n -fill x", animation_widget->GetWidgetName());

  // Create a volume property and assign it
  // We need color tfuncs, opacity, and gradient
  vtkPiecewiseFunction *opacityTransferFunction = vtkPiecewiseFunction::New();
    opacityTransferFunction->AddPoint(  0.0,  0.0);
    opacityTransferFunction->AddPoint( 90.0,  0.0);
    opacityTransferFunction->AddPoint( 137.9, 0.119);
    opacityTransferFunction->AddPoint(255.0,  0.2);

  vtkColorTransferFunction *colorTransferFunction = vtkColorTransferFunction::New();
    colorTransferFunction->AddRGBPoint(0.0,   0.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(120.0, 1.0, 1.0, 1.0);
    colorTransferFunction->AddRGBPoint(160.0, 1.0, 1.0, 0.0);
    colorTransferFunction->AddRGBPoint(200.0, 1.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(255.0, 0.0, 1.0, 1.0);

  vtkPiecewiseFunction *gradientTransferFunction = vtkPiecewiseFunction::New();
    gradientTransferFunction->AddPoint(  0.0, 0);
    gradientTransferFunction->AddPoint(  2.5, 0);
    gradientTransferFunction->AddPoint( 12.7, 1);
    gradientTransferFunction->AddPoint(255.0, 1);

  vtkVolumeProperty *volumeProperty = vtkVolumeProperty::New();
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(opacityTransferFunction);
    volumeProperty->SetGradientOpacity(gradientTransferFunction);
    volumeProperty->ShadeOff();
    volumeProperty->SetInterpolationTypeToLinear();

  vtkVolumeRayCastCompositeFunction  *compositeFunction = vtkVolumeRayCastCompositeFunction::New();

  volumeMapper = vtkVolumeRayCastMapper::New();
    volumeMapper->SetVolumeRayCastFunction(compositeFunction);
    volumeMapper->SetInputConnection(cast->GetOutputPort());

  volume = vtkVolume::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

  vtkCallbackCommand* RefreshRendererCallbackCommand = vtkCallbackCommand::New ( );
    RefreshRendererCallbackCommand->SetClientData(this);
    RefreshRendererCallbackCommand->SetCallback(vtkKWMyWindow::RefreshRenderer);

  volumePropertyWidget->SetVolumeProperty(volumeProperty);
  volumePropertyWidget->InteractiveApplyModeOn();
  volumePropertyWidget->AddObserver(vtkKWEvent::VolumePropertyChangedEvent, (vtkCommand*)RefreshRendererCallbackCommand);

  cpsel->SetColorTransferFunction(colorTransferFunction);
  cpsel->SetScalarRange(0,255);

  renderWidget->ResetCamera();
}

void vtkKWMyWindow::colorPresetSelectedCommand(const char *method)
{
  volumePropertyWidget->Update();
  renderWidget->Render();
}

void vtkKWMyWindow::refreshApplicationAfterDataLoad()
{
  //Volume parameters
  volumeMapper->SetInputConnection(cast->GetOutputPort());
  cpsel->SetScalarRange(rangeData[0], rangeData[1]);
  volumePropertyWidget->Update();

  renderWidget->AddViewProp(volume);
  renderWidget->Reset();
  renderWidget->Render();
}

void vtkKWMyWindow::openVtkFile(char *filename)
{
  vtkReader->SetFileName(filename);
  cast->SetInput((vtkDataObject*)vtkReader->GetOutput());
  vtkReader->Update();
  
  rangeData = ((vtkImageData*)vtkReader->GetOutput())->GetScalarRange();
  dimData   = ((vtkImageData*)vtkReader->GetOutput())->GetDimensions();
  
  refreshApplicationAfterDataLoad();
}

void vtkKWMyWindow::openVtkFileTestData()
{
  openVtkFile("../test_data/tooth.vtk");
}

void vtkKWMyWindow::openVtkFileDialog()
{
  vtkKWLoadSaveDialog *open_dialog = vtkKWLoadSaveDialog::New();
  open_dialog->SetParent(this->GetParentTopLevel());
  open_dialog->RetrieveLastPathFromRegistry("OpenFilePath");
  open_dialog->Create();
  open_dialog->SetTitle("Open VTK file");
  int res = open_dialog->Invoke();
  
  if (res)
  {
    char *filename = open_dialog->GetFileName();
    openVtkFile(filename);
    open_dialog->SaveLastPathToRegistry("OpenFilePath");
  }
  open_dialog->Delete();
}
