/***************************************************************************
  qgsdepthrenderview.h
  --------------------------------------
  Date                 : august 2022
  Copyright            : (C) 2022 by Benoit De Mezzo and (C) 2020 by Belgacem Nedjima
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSDEPTHRENDERVIEW_H
#define QGSDEPTHRENDERVIEW_H

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
  class QRenderCapture;
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
 * \since QGIS 3.28
 */
class QgsDepthRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:
    QgsDepthRenderView( QObject *parent, Qt3DRender::QCamera *mainCamera );

    //! Returns the render capture object used to take an image of the depth buffer of the scene
    Qt3DRender::QRenderCapture *renderCapture() { return mDepthRenderCapture; }

  private:
    Qt3DRender::QCamera *mMainCamera = nullptr;
    Qt3DRender::QRenderCapture *mDepthRenderCapture = nullptr;

    Qt3DRender::QFrameGraphNode *constructDepthRenderPass();
};

#endif // QGSDEPTHRENDERVIEW_H
