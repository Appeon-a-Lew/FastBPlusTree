#include <doctest>


/**
 * List of Tests:
 * 
 * 1) insert:
 *      normal insert
 *      duplicate
 *      too big
 *      split 
 * 
 * 2) split
 *      before after simple
 *      before after hard 
 *      split inner nodes
 * 
 * 3) lookup
 *      simple 
 *      non existent
 *      long 
 *      short
 *      multiple same query
 * 
 * 4) remove
 *      simple 
 *      non existent
 *      merge
 *      single vs multi delete
 * 
 * 5) merge 
 *      before after simple 
 *      before after hard
 *      merge inner nodes
 * 
 * 6) LowerBoundEytz vs std::lower_map from algorithms
 * c
*/