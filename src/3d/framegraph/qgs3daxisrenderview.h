/***************************************************************************
  qgs3daxisrenderview.h
  --------------------------------------
  Date                 : June 2024
  Copyright            : (C) 2024 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DAXISRENDERVIEW_H
#define QGS3DAXISRENDERVIEW_H

#include "qgis_3d.h"
#include "qgsabstractrenderview.h"
#include <QObject>

#define SIP_NO_FILE

class QColor;
class QRect;
class QSurface;

namespace Qt3DCore
{
  class QEntity;
}

namespace Qt3DRender
{
  class QRenderSettings;
  class QCamera;
  class QFrameGraphNode;
  class QLayer;
  class QViewport;
  class QSubtreeEnabler;
  class QRenderTargetSelector;
}

class Qgs3DMapCanvas;

class QgsFrameGraph;
class Qgs3DMapSettings;

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief 3d axis render view class
 *
 * \note Not available in Python bindings
 * \since QGIS 3.40
 */
class _3D_EXPORT Qgs3DAxisRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:

    /**
     * Constructor for Qgs3DAxisRenderView with the specified \a parent object.
     */
    Qgs3DAxisRenderView( QObject *parent, Qgs3DMapCanvas *canvas,
                         Qt3DRender::QCamera *objectCamera, Qt3DRender::QCamera *labelsCamera, Qgs3DMapSettings *settings );

    //! Returns the layer to be used by entities to be included in this renderview
    virtual Qt3DRender::QLayer *layerToFilter() override;

    //! Returns the viewport associated to this renderview
    virtual Qt3DRender::QViewport *viewport() override;

    //! Returns the top node of this renderview branch. Should be a QSubtreeEnabler
    virtual Qt3DRender::QFrameGraphNode *topGraphNode() override;

    //! Enable or disable via \a enable the renderview sub tree
    virtual void enableSubTree( bool enable ) override;

    //! Returns true if renderview is enabled
    virtual bool isSubTreeEnabled() override;

    Qt3DRender::QLayer *labelsLayer() { return mLabelsLayer; }

  public slots:
    //! Updates viewport horizontal \a position
    void onHorizPositionChanged( Qt::AnchorPoint position );

    //! Updates viewport vertical \a position
    void onVertPositionChanged( Qt::AnchorPoint position );

    //! Updates viewport \a size
    void onViewportSizeUpdate( int size = 0 );

  signals:
    void viewportScaleFactorChanged( double scaleFactor );

  private:
    Qgs3DMapCanvas *mCanvas;
    Qt3DRender::QCamera *mObjectCamera;
    Qt3DRender::QCamera *mLabelsCamera;
    Qt3DRender::QLayer *mObjectLayer = nullptr;
    Qt3DRender::QLayer *mLabelsLayer = nullptr;
    Qt3DRender::QViewport *mViewport = nullptr;
    Qt3DRender::QSubtreeEnabler *mRendererEnabler = nullptr;
    Qgs3DMapSettings *mMapSettings = nullptr;
    Qt3DRender::QRenderTargetSelector *mRenderTargetSelector = nullptr;

    //! Handles target outputs changes
    virtual void onTargetOutputUpdate() override;
};


#endif // QGS3DAXISRENDERVIEW_H
