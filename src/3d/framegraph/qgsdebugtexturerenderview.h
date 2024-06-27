/***************************************************************************
  qgsdebugtexturerenderview.h
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

#ifndef QGSDEBUGTEXTURERENDERVIEW_H
#define QGSDEBUGTEXTURERENDERVIEW_H

#include "qgsabstractrenderview.h"

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
 * \brief Container class that holds different objects related to debug rendering
 *
 * \note Not available in Python bindings
 *
 * The debug buffer render pass is made to copy the debug buffer into
 * an RGB texture that can be captured into a QImage and sent to the CPU for
 * calculating real 3D points from mouse coordinates (for zoom, rotation, drag..)
 *
 * \since QGIS 3.40
 */
class QgsDebugTextureRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:
    QgsDebugTextureRenderView( QObject *parent, Qt3DRender::QCamera *mainCamera );

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


  private:
    Qt3DRender::QCamera *mMainCamera = nullptr;

    Qt3DRender::QSubtreeEnabler *mRendererEnabler = nullptr;
    Qt3DRender::QLayer *mLayer = nullptr;

    Qt3DRender::QFrameGraphNode *buildRenderPass();

    //! Handles target outputs changes
    virtual void onTargetOutputUpdate() override;
};

#endif // QGSDEBUGTEXTURERENDERVIEW_H
