#ifndef QGS3DCREATEPRIMITIVESPHEREDIALOG_H
#define QGS3DCREATEPRIMITIVESPHEREDIALOG_H

#include "qgs3dcreateprimitivedialog.h"

class Qgs3DCreatePrimitiveSphereDialog : public Qgs3DCreatePrimitiveDialog
{
    Q_OBJECT
  public:
    Qgs3DCreatePrimitiveSphereDialog( Qt::WindowFlags f = ( Qt::WindowFlags() | Qt::Tool ) );

    void resetData() override;

    double defaultSize() override { return mSpinSize->value(); };

    void setRadius( double size );
    void setSegment1( int size );
    void setSegment2( int size );


  private:
    QDoubleSpinBox *mSpinSize;
    QSpinBox *mSpinSegment1;
    QSpinBox *mSpinSegment2;
};

#endif // QGS3DCREATEPRIMITIVESPHEREDIALOG_H
