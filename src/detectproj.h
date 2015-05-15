#ifndef Detectproj_H
#define Detectproj_H

#include <vector>
#include <utility>
#include <ostream>

#include "libalgo/source/structures/point/Node3DCartesianProjected.h"
#include "libalgo/source/structures/point/Node3DCartesian.h"
#include "libalgo/source/structures/point/Point3DCartesian.h"

#include "libalgo/source/structures/face/Face.h"

#include "libalgo/source/structures/projection/Projection.h"

#include "libalgo/source/structures/list/Container.h"

#include "libalgo/source/algorithms/graticule/Graticule.h"
#include "libalgo/source/algorithms/ransac/Ransac.h"
#include "libalgo/source/algorithms/cartanalysis/CartAnalysis.h"
#include "libalgo/source/algorithms/voronoi2D/Voronoi2D.h"
#include "libalgo/source/algorithms/faceoverlay/FaceOverlay.h"

#include "libalgo/source/algorithms/geneticalgorithms/DifferentialEvolution.h"

#include "libalgo/source/comparators/sortPointsByLat.h"

#include "libalgo/source/io/File.h"
#include "libalgo/source/io/GeoJSONExport.h"
#include "libalgo/source/algorithms/projectiontoproj4/ProjectionToProj4.h"

#include "libalgo/source/exceptions/Error.h"
#include "libalgo/source/exceptions/ErrorMathInvalidArgument.h"
#include "libalgo/source/exceptions/ErrorFileRead.h"

#include "libalgo/source/algorithms/transformation/HomotheticTransformation2D.h"
#include "libalgo/source/algorithms/turningfunction/TurningFunction.h"
#include "libalgo/source/algorithms/innerdistance/InnerDistance.h"

//For tests
#include "libalgo/source/algorithms/numderivative/FProjEquationDerivative2Var.h"
#include "libalgo/source/algorithms/nonlinearleastsquares/FAnalyzeProjJ.h"
#include "libalgo/source/algorithms/nonlinearleastsquares/FAnalyzeProjV.h"
#include "libalgo/source/algorithms/nonlinearleastsquares/FAnalyzeProjC.h"
#include "libalgo/source/algorithms/simplexmethod/FAnalyzeProjV2S.h"

extern const char* PROJECTIONS_FILE;

void detectproj_init();

bool detectproj(
        const std::vector<Node3DCartesian <double> *>& test_points,
        const std::vector<Point3DGeographic <double> *>& ref_points,
        std::vector<std::pair<std::string, std::string> >& proj_exports,
        std::ostream& err
);

#endif
