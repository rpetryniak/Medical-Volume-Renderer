#ifndef __vtkKWMyWindow_h
#define __vtkKWMyWindow_h

#include "vtkKWWindow.h"

class vtkKWRenderWidget;
class vtkKWColorPresetMenu;
class vtkKWVolumePropertyWidget;
class vtkVolumeRayCastMapper;
class vtkVolume;
class vtkStructuredPointsReader;
class vtkImageCast;

class vtkKWMyWindow : public vtkKWWindow
{
public:
  static vtkKWMyWindow* New();
  vtkTypeRevisionMacro(vtkKWMyWindow,vtkKWWindow);

  // Callbacks
  virtual void colorPresetSelectedCommand(const char *method);

  virtual void openVtkFileDialog();
  void openVtkFile(char *filename);
  void openVtkFileTestData();

  static void RefreshRenderer(vtkObject *caller, unsigned long eid, void *clientData, void *callData);
protected:
  vtkKWMyWindow();
  ~vtkKWMyWindow();
  
  virtual void CreateWidget();

  vtkKWRenderWidget           *renderWidget;
  vtkKWColorPresetMenu        *cpsel;
  vtkKWVolumePropertyWidget   *volumePropertyWidget;
  vtkVolumeRayCastMapper      *volumeMapper;
  vtkVolume                   *volume;
  vtkStructuredPointsReader   *vtkReader;
  vtkImageCast                *cast;
  
  int     isoLevel;
  float   isoOpacity;
  double  *rangeData;
  int     *dimData;
private:
  vtkKWMyWindow(const vtkKWMyWindow&);   // Not implemented.
  void operator=(const vtkKWMyWindow&);  // Not implemented.

  void refreshApplicationAfterDataLoad();
};

#endif
