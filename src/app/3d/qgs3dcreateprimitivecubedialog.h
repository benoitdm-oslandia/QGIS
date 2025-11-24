#ifndef QGS3DCREATEPRIMITIVECUBEDIALOG_H
#define QGS3DCREATEPRIMITIVECUBEDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveCubeDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveCubeDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;
    void setSize( double size );

    double defaultSize() override { return mSpinSize->value(); };

  private:
    QDoubleSpinBox *mSpinSize;
};

#endif // QGS3DCREATEPRIMITIVECUBEDIALOG_H
