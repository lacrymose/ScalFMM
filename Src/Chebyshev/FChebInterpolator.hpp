#ifndef FCHEBINTERPOLATOR_HPP
#define FCHEBINTERPOLATOR_HPP


#include "./FChebMapping.hpp"
#include "./FChebTensor.hpp"
#include "./FChebRoots.hpp"



/**
 * @author Matthias Messner (matthias.matthias@inria.fr)
 * Please read the license
 */

/**
 * @class FChebInterpolator
 * The class @p FChebInterpolator defines the anterpolation (M2M) and
 * interpolation (L2L) operations.
 */
template <int ORDER>
class FChebInterpolator : FNoCopyable
{
  // compile time constants and types
  enum {nnodes = TensorTraits<ORDER>::nnodes};
  typedef FChebRoots< ORDER>  BasisType;
  typedef FChebTensor<ORDER> TensorType;

  FReal T_of_roots[ORDER][ORDER];
	unsigned int node_ids[nnodes][3];
  


public:
	/**
	 * Constructor: It initializes the Chebyshev polynomials at the Chebyshev
	 * roots/interpolation point
	 */
	explicit FChebInterpolator()
	{
		// initialize chebyshev polynomials of root nodes: T_o(x_j)
    for (unsigned int o=1; o<ORDER; ++o)
      for (unsigned int j=0; j<ORDER; ++j)
        T_of_roots[o][j] = FReal(BasisType::T(o, BasisType::roots[j]));

		// initialize root node ids
		TensorType::setNodeIds(node_ids);
	}


	
	/**
	 * The anterpolation corresponds to the P2M and M2M operation. It is indeed
	 * the transposed interpolation.
	 */
	template <class ContainerClass>
	void anterpolate(const F3DPosition& center,
									 const FReal width,
									 FReal *const multipoleExpansion,
									 const ContainerClass *const sourceParticles) const
	{
		const map_glob_loc map(center, width);
		F3DPosition localPosition;
		FReal T_of_x[ORDER][3];

		typename ContainerClass::ConstBasicIterator iter(*sourceParticles);
		while(iter.hasNotFinished()){

			// map global position to [-1,1]
			map(iter.data().getPosition(), localPosition);
			
			// get source value
			const FReal sourceValue = iter.data().getPhysicalValue();

			// evaluate chebyshev polynomials of source particle: T_o(x_i)
      for (unsigned int o=1; o<ORDER; ++o) {
        T_of_x[o][0] = BasisType::T(o, localPosition.getX());
        T_of_x[o][1] = BasisType::T(o, localPosition.getY());
        T_of_x[o][2] = BasisType::T(o, localPosition.getZ());
			}

			// anterpolate
			for (unsigned int n=0; n<nnodes; ++n) {
				FReal S = FReal(1.);
				for (unsigned int d=0; d<3; ++d) {
					const unsigned int j = node_ids[n][d];
					FReal S_d = FReal(1.) / ORDER;
					for (unsigned int o=1; o<ORDER; ++o)
						S_d += FReal(2.) / ORDER * T_of_x[o][d] * T_of_roots[o][j];
					S *= S_d;
				}
				multipoleExpansion[n] += S * sourceValue;
			}
			
			iter.gotoNext();
		}
	}


	
	/**
	 * The interpolation corresponds to the L2L and L2P operation.
	 */
	template <class ContainerClass>
	void interpolate(const F3DPosition& center,
									 const FReal width,
									 const FReal *const localExpansion,
									 ContainerClass *const localParticles) const
	{
		const map_glob_loc map(center, width);
		F3DPosition localPosition;
		FReal T_of_x[ORDER][3];
		
		typename ContainerClass::BasicIterator iter(*localParticles);
		while(iter.hasNotFinished()){
			
			// map global position to [-1,1]
			map(iter.data().getPosition(), localPosition);
			
			// get target value
			FReal targetValue = iter.data().getPhysicalValue();

			// evaluate chebyshev polynomials of source particle: T_o(x_i)
      for (unsigned int o=1; o<ORDER; ++o) {
        T_of_x[o][0] = BasisType::T(o, localPosition.getX());
        T_of_x[o][1] = BasisType::T(o, localPosition.getY());
        T_of_x[o][2] = BasisType::T(o, localPosition.getZ());
			}

			// interpolate and increment target value
			for (unsigned int n=0; n<nnodes; ++n) {
				FReal S = FReal(1.);
				for (unsigned int d=0; d<3; ++d) {
					const unsigned int j = node_ids[n][d];
					FReal S_d = FReal(1.) / ORDER;
					for (unsigned int o=1; o<ORDER; ++o)
						S_d += FReal(2.) / ORDER * T_of_x[o][d] * T_of_roots[o][j];
					S *= S_d;
				}
				targetValue += S * localExpansion[n];
			}

			iter.data().setPhysicalValue(targetValue);

			iter.gotoNext();
		}
	}

};









///**
// * Interpolation operator setter of (N)on (L)eaf clusters. Since the grid is
// * regular, all non leaf clusters of the same level have identical
// * interpolation operators. Hence, memory for the interpolation operator S is
// * only allocated here but not freed.
// */
//template <typename cluster_type,
//          typename clusterbasis_type>
//class IOsetterNL
//  : public std::unary_function<cluster_type, void>
//{
//  enum {dim    = cluster_type::dim,
//        nboxes = cluster_type::nboxes,
//        order  = clusterbasis_type::order,
//        nnodes = clusterbasis_type::nnodes};
//
//  typedef typename clusterbasis_type::value_type value_type;
//  typedef typename PointTraits<dim>::point_type  point_type;
//
//  typedef Chebyshev< order>  basis_type;
//  typedef Tensor<dim,order> tensor_type; 
//
//  boost::shared_array<value_type> S;
//
//  std::vector<clusterbasis_type>& basis;
//  
//public:
//  explicit IOsetterNL(std::vector<clusterbasis_type>& _basis,
//                      const double  ext,
//                      const double cext)
//    : S(new value_type [nboxes*nnodes * nnodes]), basis(_basis)
//  {
//    // some factors
//    const double frac       = cext / ext;
//    const double radius     = 1. - frac;
//    const double cextension = 2. * frac; 
//
//    // setup interpolation nodes of child clusters
//    point_type x [nboxes*nnodes];
//    for (unsigned int b=0; b<nboxes; ++b) {
//      point_type center;
//      for (unsigned int d=0; d<dim; ++d)
//        center[d] = radius * DimTraits<dim>::child_pos[b][d];
//      const map_loc_glob<dim> map(center, cextension);
//      
//      for (unsigned int n=0; n<nnodes; ++n)
//        for (unsigned int d=0; d<dim; ++d)
//          x[b*nnodes + n][d]
//            = map(d, basis_type::nodes[tensor_type::node_ids[n][d]]);
//    }
//
//    // compute S
//    FChebInterpolator<dim,order,value_type> cmp;
//    value_type S_[nnodes];
//    for (unsigned int b=0; b<nboxes; ++b)
//      for (unsigned int i=0; i<nnodes; ++i) {
//        cmp(x[b*nnodes + i], S_);
//        HYENA_ASSERT(check_nan(nnodes, S_));
//        for (unsigned int j=0; j<nnodes; ++j)
//          S[b * nnodes*nnodes + j*nnodes + i] = S_[j];
//      }
//  }
//  
//  void operator()(cluster_type *const cl) const
//  {
//    HYENA_ASSERT(!cl->level->isleaf());
//    basis.at(cl->cidx).assign(S);
//  }
//
//};



#endif
