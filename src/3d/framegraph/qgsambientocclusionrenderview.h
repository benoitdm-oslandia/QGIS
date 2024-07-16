/***************************************************************************
  qgsambientocclusionrenderview.h
  --------------------------------------
  Date                 : June 2024
  Copyright            : (C) 2024 by Benoit De Mezzo and (C) 2020 by Belgacem Nedjima
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSAMBIENTOCCLUSIONRENDERVIEW_H
#define QGSAMBIENTOCCLUSIONRENDERVIEW_H

#include "qgsabstractrenderview.h"
#include "qgsambientocclusionrenderentity.h"
#include "qgsambientocclusionblurentity.h"

namespace Qt3DRender
{
  class QRenderSettings;
  class QLayer;
  class QSubtreeEnabler;
  class QTexture2D;
  class QCamera;
  class QCameraSelector;
  class QLayerFilter;
  class QRenderTargetSelector;
  class QClearBuffers;
  class QRenderStateSet;
}

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief Container class that holds different objects related to depth rendering
 *
 * \note Not available in Python bindings
 *
 * The depth buffer render pass is made to copy the depth buffer into
 * an RGB texture that can be captured into a QImage and sent to the CPU for
 * calculating real 3D points from mouse coordinates (for zoom, rotation, drag..)
 *
 * \since QGIS 3.40
 */
class QgsAmbientOcclusionRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:
    QgsAmbientOcclusionRenderView( QObject *parent
                                   , Qt3DRender::QCamera *mainCamera
                                   , QSize mSize
                                   , Qt3DRender::QTexture2D *forwardDepthTexture
                                   , Qt3DCore::QEntity *rootSceneEntity );

    //! Enable or disable via \a enable the renderview sub tree
    virtual void enableSubTree( bool enable ) override;

    //! Delegates to QgsAmbientOcclusionRenderEntity::setIntensity
    void setIntensity( float intensity );

    //! Delegates to QgsAmbientOcclusionRenderEntity::setRadius
    void setRadius( float radius );

    //! Delegates to QgsAmbientOcclusionRenderEntity::setThreshold
    void setThreshold( float threshold );

    virtual void updateTargetOutputSize( int width, int height ) override;

  private:
    Qt3DRender::QCamera *mMainCamera = nullptr;

    Qt3DRender::QLayer *mAOPassLayer = nullptr;
    Qt3DRender::QTexture2D *mAOPassTexture = nullptr;
    Qt3DRender::QLayer *mBlurPassLayer = nullptr;

    Qt3DRender::QFrameGraphNode *buildRenderPass( QSize mSize, Qt3DRender::QTexture2D *forwardDepthTexture
        , Qt3DCore::QEntity *rootSceneEntity );

    QgsAmbientOcclusionRenderEntity *mAmbientOcclusionRenderEntity = nullptr;
    QgsAmbientOcclusionBlurEntity *mAmbientOcclusionBlurEntity = nullptr;
};

#endif // QGSAMBIENTOCCLUSIONRENDERVIEW_H
