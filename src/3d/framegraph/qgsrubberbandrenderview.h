/***************************************************************************
  qgsrubberbandrenderview.h
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

#ifndef QGSRUBBERBANDRENDERVIEW_H
#define QGSRUBBERBANDRENDERVIEW_H

#include "qgsabstractrenderview.h"

#define SIP_NO_FILE

/**
 * \ingroup 3d
 * \brief Container class that holds different objects related to rubberband rendering
 *
 * \note Not available in Python bindings
 *
 * The rubberband buffer render pass is made to copy the rubberband buffer into
 * an RGB texture that can be captured into a QImage and sent to the CPU for
 * calculating real 3D points from mouse coordinates (for zoom, rotation, drag..)
 *
 * \since QGIS 3.28
 */
class QgsRubberBandRenderView : public QgsAbstractRenderView
{
    Q_OBJECT
  public:
    QgsRubberBandRenderView( QObject *parent, Qt3DRender::QCamera *mainCamera, Qt3DCore::QEntity *rootSceneEntity );

    //! Returns entity for all rubber bands (to show them always on top)
    Qt3DCore::QEntity *rubberBandEntity() const { return mRubberBandsRootEntity;}

  private:
    Qt3DRender::QCamera *mMainCamera = nullptr;
    Qt3DCore::QEntity *mRubberBandsRootEntity = nullptr;

    Qt3DRender::QFrameGraphNode *constructRenderPass();  //#spellok
};

#endif // QGSRUBBERBANDRENDERVIEW_H
