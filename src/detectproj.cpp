#include "detectproj.h"


const char* PROJECTIONS_FILE = "/usr/local/detectproj/projections.txt";

Container <Projection <double> *> proj_list;

void detectproj_init() {
    proj_list.load(PROJECTIONS_FILE);
}

bool detectproj(const std::vector<Node3DCartesian <double> *>& test_points, const std::vector<Point3DGeographic <double> *> ref_points, std::ostream& out, std::ostream& err) {
    std::ofstream blackhole("/dev/null");
    TAnalysisParameters <double> analysis_parameters;
    TAnalyzedProjParametersList <double> ::Type analyzed_proj_parameters_list;

    analysis_parameters.analyze_normal_aspect = true;
    analysis_parameters.analyze_transverse_aspect = true;
    analysis_parameters.analyze_oblique_aspect = false;
    analysis_parameters.analysis_type.a_cnd = true;
    analysis_parameters.analysis_type.a_homt = true;
    analysis_parameters.analysis_type.a_helt = true;
    analysis_parameters.analysis_type.a_gn_tf = false;
    analysis_parameters.analysis_type.a_vd_tf = true;
    analysis_parameters.analysis_type.a_vd_id = true;

    Container <Node3DCartesian <double> *> nl_test;
    Container <Point3DGeographic <double> *> nl_reference;
    nl_test.loadFromVector(test_points);
    nl_reference.loadFromVector(ref_points);

    const unsigned int n_test = nl_test.size(), n_reference = nl_reference.size();
    if ( n_test != n_reference ) {
        err << "ErroBadData: different number of test and reference points";
        return false;
    }
    //Find and remove duplicate points
    nl_test.removeDuplicateElements(nl_test.begin(), nl_test.end(),  sortPointsByX(), isEqualPointByPlanarCoordinates<Node3DCartesian <double> *>());
    nl_reference.removeDuplicateElements(nl_reference.begin(), nl_reference.end(), sortPointsByLat(), isEqualPointByPlanarCoordinates<Point3DGeographic <double> *>());

    //Total unique points
    const unsigned int n_test_unique = nl_test.size(), n_reference_unique = nl_reference.size();

    //Not enough unique points
    if ( n_test_unique < 3 ) {
        err << "ErroBadData: insufficient number of unique points ( at least 3 )";
        return false;
    }

    if ( n_test_unique != n_reference_unique ) {
        err << "ErroBadData: number of unique points in both files different";
        return false;
    }

    //Lat/lon values outside the limits
    for ( unsigned int i = 0; i < n_test_unique; i++ ) {
        if ( ( fabs ( nl_reference[i]->getLat() ) > MAX_LAT ) || ( fabs ( nl_reference[i]->getLon() ) > MAX_LON ) ) {
            err << "ErroBadData: latitude or longitude outside intervals";
            return false;
        }
    }

    for ( TAnalyzedProjParametersList <double> ::Type ::const_iterator i_analyzed_projections = analyzed_proj_parameters_list.begin(); i_analyzed_projections != analyzed_proj_parameters_list.end(); ++ i_analyzed_projections )
    {
        //Compare with all projections stored in the list of projections
        for ( TItemsList <Projection <double> *> ::Type ::const_iterator i_projections = proj_list.begin(); i_projections != proj_list.end(); ++ i_projections )
        {
            //Compare analyzed projection name with names of projections stored in the list
            if ( !strcmp ( ( * i_analyzed_projections ).proj_name, ( *i_projections )->getProjectionName() ) )
            {
                //Clone analyzed projection
                Projection <double> *analyzed_proj = ( *i_projections )->clone();

                //Set analyzed projections properties: if not set by user in command line, use default projection values from configuration file
                //We set lat0 in command line and analyzed projection does not define own lat0 in configuration file
                if ( ( fabs ( ( * i_analyzed_projections ).lat0 ) <= MAX_LAT0 ) && ( analyzed_proj->getLat0() == 45.0 ) )
                    analyzed_proj->setLat0 ( ( * i_analyzed_projections ).lat0 );

                //We set latp, lonp in command and analyzed projection does not define own latp,lonp in configuration file (have a normal aspect)
                if ( ( fabs ( ( * i_analyzed_projections ).latp ) < MAX_LAT ) && ( fabs ( ( * i_analyzed_projections ).lonp ) <= MAX_LON ) && ( analyzed_proj->getCartPole().getLat() == MAX_LAT ) )
                {
                    Point3DGeographic <double> cart_pole ( ( * i_analyzed_projections ).latp, ( * i_analyzed_projections ).lonp );
                    analyzed_proj->setCartPole ( cart_pole );
                }

                //Add projection to the list of analyzed projections
                analysis_parameters.analyzed_projections.push_back ( analyzed_proj );
            }
        }
    }

    //Find meridians and parallels using RANSAC
    TMeridiansList <double> ::Type meridians;
    TParallelsList <double> ::Type parallels;

    //Precompute geometric structures for test dataset: Create Voronoi diagram and merge Voronoi cells
    Container <HalfEdge <double> *> hl_dt_test, hl_vor_test, hl_merge_test;
    Container <Node3DCartesian <double> *> nl_vor_test, intersections_test;
    Container <VoronoiCell <double> *> vor_cells_test;
    Container <Face <double> *> faces_test;

    //Precompute Voronoi diagram for the test dataset and merge Voronoi cells: only if outliers are not removed
    if ( analysis_parameters.analysis_type.a_vd_tf )
    {
        try
        {
            //Compute Voronoi cells with full topology
            Voronoi2D::VD ( ( Container <Node3DCartesian <double> *> & ) nl_test, nl_vor_test, hl_dt_test, hl_vor_test, vor_cells_test, AppropriateBoundedCells, TopologicApproach, 0, false, NULL );
            //Are there enough Voronoi cells for analysis? If not, disable analysis
            analysis_parameters.analysis_type.a_vd_tf = ( faces_test.size() < MIN_BOUNDED_VORONOI_CELLS );
            //There is enough Voronoi cells for analysis
            if ( analysis_parameters.analysis_type.a_vd_tf )
            {
                //Merge Voronoi cells with all adjacent cells
                for ( TItemsList <Node3DCartesian <double> *> ::Type ::iterator i_points_test = nl_test.begin(); i_points_test != nl_test.end() ; i_points_test ++ )
                {
                    //Get Voronoi cell
                    VoronoiCell <double> *vor_cell_test = dynamic_cast < VoronoiCell <double> * > ( ( *i_points_test ) -> getFace() );
                    //Merge Voronoi cell only if exists and it is bounded
                    Face <double> * face_test = NULL;
                    if ( ( vor_cell_test != NULL )  && ( vor_cell_test->getBounded() ) )
                    {
                        Voronoi2D::mergeVoronoiCellAndAdjacentCells ( vor_cell_test, &face_test, intersections_test, hl_merge_test );
                    }
                    //Add face to the list
                    faces_test.push_back ( face_test );
                }
            }
        }
        //Throw exception
        catch ( Error & error )
        {
            err << error.getExceptionText();
            return false;
        }
    }

    //Compute cartometric analysis_type and create samples
    //Performed + thrown samples = total samples
    unsigned int total_created_or_thrown_samples = 0;
    Container <Sample <double> > sl;
    //Repeat analysis with the different sensitivity, if neccessary
    for ( unsigned int i = 0; ( i < analysis_parameters.analysis_repeat + 1 ) && ( analysis_parameters.heuristic_sensitivity_ratio < MAX_HEURISTIC_SENSTIVITY_RATIO ) &&
            ( sl.size() == 0 ); i++, analysis_parameters.heuristic_sensitivity_ratio += 2 )
    {
        //Perform analysis: simplex method
        if (analysis_parameters.analysis_method == SimplexMethod || analysis_parameters.analysis_method == SimplexRotMethod || analysis_parameters.analysis_method == SimplexRot2Method || analysis_parameters.analysis_method == SimplexShiftsMethod)
        {
            CartAnalysis::computeAnalysisForAllSamplesSim ( sl, proj_list, nl_test, nl_reference, meridians, parallels, faces_test, analysis_parameters,
                total_created_or_thrown_samples, &blackhole );
        }

        //Perform analysis: differential evolution
        else if (analysis_parameters.analysis_method == DifferentialEvolutionMethod || analysis_parameters.analysis_method == DifferentialEvolutionRotMethod || analysis_parameters.analysis_method == DifferentialEvolutionRot2Method || analysis_parameters.analysis_method == DifferentialEvolutionShiftsMethod)
        {
            CartAnalysis::computeAnalysisForAllSamplesDE ( sl, proj_list, nl_test, nl_reference, meridians, parallels, faces_test, analysis_parameters,
                total_created_or_thrown_samples, &blackhole );
        }

        //Perform analysis: minimum least squares
        else
        {
            CartAnalysis::computeAnalysisForAllSamplesMLS ( sl, proj_list, nl_test, nl_reference, meridians, parallels, faces_test, analysis_parameters,
                total_created_or_thrown_samples, &blackhole );
        }
    }

    //No sample had been computed and added
    if ( sl.size() == 0 )
    {
        if ( analysis_parameters.analysis_repeat == 0 ) {
            err << "ErrorBadData: no sample had been computed. Try to increase heuristic sensitivity ratio \"sens\", disable heuristic or check projection equations.";
            return false;
        }
        else {
            err << "ErrorBadData: no sample had been computed. Disable heuristic or check projection equations.";
            return false;
        }
    }

    //Sort computed results
    CartAnalysis::sortSamplesByComputedRatios ( sl, analysis_parameters.analysis_type );

    //Create graticules
    //Get a sample projection
    Projection <double> *proj = sl[0].getProj();
    //Set properties for a projection
    proj->setR ( sl[0].getR() );
    Point3DGeographic <double> cart_pole ( sl[0].getLatP(), sl[0].getLonP() );
    proj->setCartPole ( cart_pole );
    proj->setLat0 ( sl[0].getLat0() );
    proj->setLon0 ( sl[0].getLon0() );
    proj->setDx ( sl[0].getDx() );
    proj->setDy ( sl[0].getDy() );
    proj->setC ( sl[0].getC() );

    //Get limits
    TMinMax <double> lon_interval ( ( * std::min_element ( nl_reference.begin(), nl_reference.end(), sortPointsByLon () ) )->getLon(),
            ( * std::max_element ( nl_reference.begin(), nl_reference.end(), sortPointsByLon () ) )->getLon() );
    TMinMax <double> lat_interval ( ( * std::min_element ( nl_reference.begin(), nl_reference.end(), sortPointsByLat () ) )->getLat(),
            ( * std::max_element ( nl_reference.begin(), nl_reference.end(), sortPointsByLat () ) )->getLat() );

    //Create data structures for the graticule representation
    unsigned int index = 0;
    TMeridiansListF <double> ::Type meridians_exp;
    TParallelsListF <double> ::Type parallels_exp;
    Container <Node3DCartesian <double> *> mer_par_points;

    //Set font height
    const double font_height = 0.05 * proj->getR() * std::min ( analysis_parameters.lat_step, analysis_parameters.lon_step ) * M_PI / 180;

    //Create graticule
    double alpha = sl[0].getAlpha();
    Graticule::computeGraticule ( proj, lat_interval, lon_interval, analysis_parameters.lat_step, analysis_parameters.lon_step, 0.1 * analysis_parameters.lat_step, 0.1 * analysis_parameters.lon_step, alpha, TransformedGraticule, meridians_exp, parallels_exp, &mer_par_points, index );

    //Export graticule into GeoJSON
    GeoJSONExport::exportGraticule( out, meridians_exp, parallels_exp, mer_par_points, font_height, analysis_parameters.lat_step, analysis_parameters.lon_step );

    blackhole.close();
    return true;
}
