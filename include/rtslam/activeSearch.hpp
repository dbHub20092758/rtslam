/**
 * \file activeSearch.hpp
 *
 *  Created on: 10/04/2010
 *     \author: jsola@laas.fr
 *
 *
 *  ## Add a description here ##
 *
 * \ingroup rtslam
 */

#ifndef ACTIVESEARCH_HPP_
#define ACTIVESEARCH_HPP_

#include "jmath/random.hpp"
#include "rtslam/gaussian.hpp"
#include "jmath/jblas.hpp"
#include <iostream>

namespace jafar {
	namespace rtslam {
		using namespace jblas;
		using namespace std;

		/**
		 * Region of Interest class.
		 *
		 * This class stores an upper-left corner and a size of a region of interest (ROI).
		 * \ingroup rtslam
		 */
		class ROI {
			public:
				veci2 upleft_;
				veci2 size_;
				ROI(const int ulx = 0, const int uly = 0, const int sx = 0, const int sy = 0) {
					upleft_(0) = ulx;
					upleft_(1) = uly;
					size_(0) = sx;
					size_(1) = sy;
				}
				veci2 downright() {
					return upleft_ + size_;
				}
				veci2 & upleft() {
					return upleft_;
				}
				veci2 & size() {
					return size_;
				}

		};

		/**
		 * Active search tesselation grid.
		 *
		 * \author jsola@laas.fr
		 *
		 * This class implements a tesselation grid for achieving active search
		 * behavior in landmark initialization.
		 *
		 * The grid defines a set of cells in the image.
		 * The idea is to count the number of projected landmarks per grid cell,
		 * and use one randomly chosen cell that is empty
		 * for feature detection and landmark initialization.
		 * This guarantees that the system will automatically populate all the
		 * regions of the image.
		 *
		 * The feature density can be controlled by
		 * adjusting the grid's number of cells.
		 * Typically, use grids of 5x5 to 9x9 cells.
		 *
		 * This class implements a few interesting features:
		 * - The grid can be randomly re-positioned at each frame to avoid dead zones at the cell edges.
		 * - Only the inner cells are activated for feature detection to avoid reaching the image edges.
		 * - The region of interest (ROI) associated with a particular cell is shrinked with a parametrizable margin
		 *   to guarantee a minimum separation between existing and new features.
		 *
		 * The blue and green grids in the figure below represent the grid
		 * at two different offsets, corresponding to two different frames.
		 *
		 * 	\image html tesselationGrid.png "The tesselation grid used for active feature detection and initialization"
		 *
		 * This second figure shows a typical situation that we use to explain the basic mechanism.
		 *
		 * \image html tesselationExample.png "A typical configuration of the tesselation grid"
		 *
		 * Observe the figure and use the following facts as an operation guide:
		 * - The grid is offset by a fraction of a cell size.
		 * 		- use renew() at each frame to clear the grid and set a new offset.
		 * - Projected landmarks are represented by red dots.
		 * 		- After projection, use addPixel() to add a new dot to the grid.
		 * - Cells with projected landmarks inside are 'occupied'.
		 * - Only inner cells are considered (thick blue rectangle).
		 * - One cell is chosen randomly among those that are empty.
		 * - The Region of Interest (ROI) is smaller than the cell to guarantee a minimum feature separation.
		 * 		- Use the optional 'separation' parameter at construction time to control this separation.
		 * 		- Use getROI() to obtain an empty ROI for initialization.
		 * - A new feature is to be be searched inside this ROI.
		 * - If you need to search more than one feature per frame, proceed like this:
		 * 		- At successful detection, add the detected pixel with addPixel().
		 * 		- Call getROI() again.
		 * 		- Repeat these two steps for each feature to be searched.
		 *
		 * We include here a schematic active-search pseudo-code algorithm to illustrate its operation:
		 * \code
		 * // Init necessary objects
		 * ActiveSearch activeSearch;
		 * ActiveSearchGrid grid(640, 480, 4, 4, 10); // Construction with 10 pixels separation.
		 *
		 * // We start projecting everybody
		 * for (obs = begin(); obs != end(); obs++)   // loop observations
		 * {
		 *   obs->project();
		 *   if (obs->isVisible())
		 *     grid.addPixel(obs->expectation.x());   // add only visible landmarks
		 * }
		 *
		 * // Then we process the selected observations
		 * activeSearch.selectObs();                  // select interesting features
		 * for (activeSearch.selectedObs);            // loop selected obs
		 *   obs.process();                           // process observation
		 *
		 * // Now we go to initialization
		 * grid.getROI(roi);                          // roi is now region of interest
		 * if (detectFeature(roi))                    // detect inside ROI
		 *   initLandmark();                          // initialize only if successful detection
		 * \endcode
		 *
		 * \ingroup rtslam
		 */
		class ActiveSearchGrid {

				friend ostream& operator <<(ostream & s, ActiveSearchGrid & grid);

			private:
				veci2 imgSize;
				veci2 gridSize;
				veci2 cellSize;
				veci2 offset;
				mati projectionsCount;
				mati emptyCellsTile_tmp;
				int separation;

			public:
				/**
				 * Constructor.
				 * \param imgSize_h horizontal image size, in pixels.
				 * \param imgSize_v vertical image size.
				 * \param nCells_h horizontal number of cells per image width.
				 * \param nCells_v vertical number of cells per image height.
				 * \param separation minimum separation between existing and new points.
				 */
				ActiveSearchGrid(const int & imgSize_h, const int & imgSize_v, const int & nCells_h, const int & nCells_v,
				    const int & separation = 0);

				/**
				 * Clear grid.
				 * Sets all cell counters to zero.
				 */
				void clear();

				/**
				 * Clear grid and position it at a new random location.
				 * Sets all cell counters to zero and sets a new random grid position.
				 */
				void renew();

				/**
				 * Add a projected pixel to the grid.
				 * \param pix the pixel to add.
				 */
				void addPixel(const vec2 & pix);

				/**
				 * Get ROI of a random empty cell.
				 * \param roi the resulting ROI
				 * \return true if ROI exists.
				 */
				bool getROI(ROI & roi);

			private:
				/**
				 * increment cell counter
				 */
				void addToCell(const veci2 & cell);

				/**
				 * Get cell corresponding to pixel
				 */
				template<class Vec2>
				veci2 pix2cell(const Vec2 & pix) {
					veci2 cell;
					cell(0) = (pix(0) - offset(0)) / cellSize(0);
					cell(1) = (pix(1) - offset(1)) / cellSize(1);
					return cell;
				}

				/**
				 * Get cell origin (exact pixel)
				 */
				veci2 cellOrigin(const veci2 & cell);

				/**
				 * Get cell center (can be decimal if size of cell is an odd number of pixels)
				 */
				vec2 cellCenter(const veci2 & cell);

				/**
				 * Is Cell empty?
				 */
				bool isEmpty(const veci2 & cell);

				/**
				 * Get one random empty cell
				 */
				bool pickEmptyCell(veci2 & cell);

				/**
				 * Get the region of interest, reduced by a margin.
				 */
				void cell2roi(const veci2 & cell, ROI & roi);

		};

		/**
		 * Class for active search algorithms.
		 * \ingroup rtslam
		 */
		class ActiveSearch {
			public:

		};

	}
}

#endif /* ACTIVESEARCH_HPP_ */