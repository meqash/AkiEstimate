//
//    AkiEstimate : A method for the joint estimation of Love and Rayleigh surface wave
//    dispersion from ambient noise cross-correlations.
//
//      Hawkins R. and Sambridge M., "An adjoint technique for estimation of interstation phase
//    and group dispersion from ambient noise cross-correlations", BSSA, 2019
//    
//    Copyright (C) 2014 - 2018 Rhys Hawkins
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//

#include <exception>

#include <stdio.h>
#include <getopt.h>

#include "likelihood.hpp"
#include "autosigma.hpp"

#include "simple.hpp"
#include "quasinewton.hpp"

static char short_options[] = "i:C:r:f:F:R:V:X:S:o:s:p:b:t:P:e:N:QM:h";
static struct option long_options[] = {
  {"input", required_argument, 0, 'i'},
  {"phase", required_argument, 0, 'C'},

  {"fmin", required_argument, 0, 'f'},
  {"fmax", required_argument, 0, 'F'},

  {"reference", required_argument, 0, 'r'},

  {"sigma-rho", required_argument, 0, 'R'},
  {"sigma-vs", required_argument, 0, 'V'},
  {"sigma-xi", required_argument, 0, 'X'},
  {"sigma-vpvs", required_argument, 0, 'S'},

  {"output", required_argument, 0, 'o'},

  {"scale", required_argument, 0, 's'},
  {"order", required_argument, 0, 'p'},
  {"boundaryorder", required_argument, 0, 'b'},
  {"threshold", required_argument, 0, 't'},
  {"high-order", required_argument, 0, 'P'},

  {"nsteps", required_argument, 0, 'N'},
  {"epsilon", required_argument, 0, 'e'},

  {"posterior", no_argument, 0, 'Q'},
  
  {"mode", required_argument, 0, 'M'},

  {"help", no_argument, 0, 'h'},
  
  {0, 0, 0, 0}
};

static void usage(const char *pname);

static bool invert(DispersionData &data,
                   model_t &model,
		   model_t &reference,
                   const double *damping,
                   bool posterior,
                   mesh_t &mesh,
                   lovesolver_t &love,
                   double threshold,
                   double order,
                   double highorder,
                   double boundaryorder,
                   double scale,
                   double epsilon,
                   int maxiterations,
		   int mode);

int main(int argc, char *argv[])
{
  int c;
  int option_index;
  
  char *input_file;
  char *reference_file;
  char *output_file;
  char *phase_file;

  double fmin;
  double fmax;
  
  double threshold;
  int order;
  int highorder;
  int boundaryorder;
  double scale;

  int maxiterations;
  double epsilon;
  double damping[4];

  bool nodata;

  char filename[1024];

  int mode;

  //
  // Defaults
  //
  input_file = nullptr;
  phase_file = nullptr;
  reference_file = nullptr;
  output_file = nullptr;
  threshold = 0.0;

  fmin = 1.0/40.0;
  fmax = 1.0/2.0;

  order = 5;
  highorder = 5;
  boundaryorder = 5;

  scale = 1.0e-4;

  maxiterations = 5;
  epsilon = 1.0;

  damping[0] = 0.0;
  damping[1] = 0.0;
  damping[2] = 0.0;
  damping[3] = 0.0;

  nodata = false;

  mode = 0;
  
  //
  // Command line parameters
  //
  option_index = 0;
  while (true) {

    c = getopt_long(argc, argv, short_options, long_options, &option_index);
    if (c == -1) {
      break;
    }

    switch (c) {

    case 'i':
      input_file = optarg;
      break;

    case 'C':
      phase_file = optarg;
      break;

    case 'r':
      reference_file = optarg;
      break;
      
    case 'o':
      output_file = optarg;
      break;

    case 'R':
      damping[0] = atof(optarg);
      if (damping[0] < 0.0) {
	fprintf(stderr, "error: rho std-dev must be 0 or greater\n");
	return -1;
      }
      break;

    case 'V':
      damping[1] = atof(optarg);
      if (damping[1] < 0.0) {
	fprintf(stderr, "error: vs std-dev must be 0 or greater\n");
	return -1;
      }
      break;

    case 'X':
      damping[2] = atof(optarg);
      if (damping[2] < 0.0) {
	fprintf(stderr, "error: xi std-dev must be 0 or greater\n");
	return -1;
      }
      break;

    case 'S':
      damping[3] = atof(optarg);
      if (damping[3] < 0.0) {
	fprintf(stderr, "error: vp/vs std-dev must be 0 or greater\n");
	return -1;
      }
      break;

    case 's':
      scale = atof(optarg);
      if (scale <= 0.0) {
        fprintf(stderr, "error: scale must be positive\n");
        return -1;
      }
      break;

    case 'p':
      order = atoi(optarg);
      if (order < 1) {
        fprintf(stderr, "error: order must be 1 or greater\n");
        return -1;
      }
      break;
      
    case 'b':
      boundaryorder = atoi(optarg);
      if (boundaryorder < 1) {
        fprintf(stderr, "error: boundary order must be 1 or greater\n");
        return -1;
      }
      break;

    case 't':
      threshold = atof(optarg);
      break;

    case 'P':
      highorder = atoi(optarg);
      if (highorder < 1) {
        fprintf(stderr, "error: high order must be 1 or greater\n");
        return -1;
      }
      break;

    case 'N':
      maxiterations = atoi(optarg);
      if (maxiterations < 1) {
        fprintf(stderr, "error: need at least one iteration\n");
        return -1;
      }
      break;

    case 'e':
      epsilon = atof(optarg);
      if (epsilon <= 0.0) {
        fprintf(stderr, "error: epsilon must be positive\n"); 
        return -1;
      }
      break;

    case 'Q':
      nodata = true;
      break;

    case 'M':
      mode = atoi(optarg);
      if (mode < 0 || mode > 1) {
	fprintf(stderr, "error: mode must be 0 (simple gradient desc.) or 1 (q-newton)\n");
	return -1;
      }
      break;

    default:
      fprintf(stderr, "unknown option %c\n", c);
    case 'h':
      usage(argv[0]);
      return -1;
    }
  }

  if (input_file == nullptr) {
    fprintf(stderr, "error: missing input file paramter\n");
    return -1;
  }

  if (phase_file == nullptr) {
    fprintf(stderr, "error: missing input phase parameter\n");
    return -1;
  }

  if (reference_file == nullptr) {
    fprintf(stderr, "error: missing reference file parameter\n");
    return -1;
  }

  if (output_file == nullptr) {
    fprintf(stderr, "error: missing output file parameter\n");
    return -1;
  }

  DispersionData data(fmin, fmax);

  if (!data.load(input_file)) {
    return -1;
  }

  if (!data.load_phase(phase_file)) {
    return -1;
  }

  printf("Desired range: %10.6f %10.6f\n", data.freq[data.ffirst], data.freq[data.flast]);
  data.initialise_target();
  printf("Actual  range: %10.6f %10.6f\n", data.freq[data.ffirst], data.freq[data.flast]);

    
  Mesh<double, MAXORDER> mesh;
  MeshAmplitude<double, MAXORDER, MAXORDER> amplitude;

  //
  // Load reference model
  //
  ReferenceModel reference;
  bool promote = true;
  size_t promote_order = order;
  
  if (!reference.load(reference_file, promote, promote_order)) {
    fprintf(stderr, "error: failed to load model from %s\n", reference_file);
    return -1;
  }

  sprintf(filename, "%s.initial-model", output_file);
  if (!reference.model.save(filename)) {
    fprintf(stderr, "error: failed to save initial model\n");
    return -1;
  }
  sprintf(filename, "%s.test-initial-model", output_file);
  if (!reference.reference.save(filename)) {
    fprintf(stderr, "error: failed to save initial model\n");
    return -1;
  }

  LoveMatrices<double, MAXORDER, BOUNDARYORDER> love;

  if (!invert(data,
	      reference.model,
	      reference.reference,
	      damping,
	      nodata,
	      mesh,
	      love,
	      threshold,
	      order,
	      highorder,
	      boundaryorder,
	      scale,
	      epsilon,
	      maxiterations,
	      mode)) {
    fprintf(stderr, "error: failed to invert\n");
    return -1;
  }
  
  //
  // Save model
  //
  sprintf(filename, "%s.model", output_file);
  if (!reference.model.save(filename)) {
    fprintf(stderr, "error: failed to save model\n");
    return -1;
  }

  //
  // Save predictions
  //
  sprintf(filename, "%s.pred", output_file);
  if (!data.save_predictions(filename)) {
    fprintf(stderr, "error: failed to save predictions\n");
    return -1;
  }

  return 0;
}

void usage(const char *pname)
{
  fprintf(stderr,
          "usage: %s [options]\n"
          "where options is one or more of:\n"
          "\n"
          " -i|--input <filename>           Input model (required)\n"
          " -f|--frequency <float>          Frequency\n"
          " -s|--scale <float>              Laguerre scaling (initial)\n"
          "\n"
          " -h|--help                       Show usage information\n"
          "\n",
          pname);
}


static bool invert(DispersionData &data,
                   model_t &model,
		   model_t &reference,
                   const double *damping,
                   bool posterior,
                   mesh_t &mesh,
                   lovesolver_t &love,
                   double threshold,
                   double order,
                   double highorder,
                   double boundaryorder,
                   double scale,
                   double _epsilon,
                   int maxiterations,
		   int mode)
{
  Spec1DMatrix<double> dkdp;
  Spec1DMatrix<double> dUdp;
  Spec1DMatrix<double> dLdp;

  Spec1DMatrix<int> model_mask;
  Spec1DMatrix<double> model_v;
  Spec1DMatrix<double> model_v_proposed;

  Spec1DMatrix<double> G;
  Spec1DMatrix<double> model_0;

  Spec1DMatrix<double> Cd;
  Spec1DMatrix<double> Cm;

  Spec1DMatrix<double> residuals;

  double epsilon[2];
  LeastSquaresIterator *step[2];

  double PRIOR_MIN[4] = {0.1e3, 0.5e3, 0.5, 1.0};
  double PRIOR_MAX[4] = {8.0e3, 10.0e3, 1.5, 2.5};
  

  epsilon[0] = _epsilon;
  epsilon[1] = _epsilon;
  
  step[0] = new SimpleStep();
  step[1] = new QuasiNewton();

  double frequency_thin = 0.001;

  double like = likelihood_love(data,
				model,
				reference,
				damping,
				posterior,
				mesh,
				love,
				dkdp,
				dUdp,
				dLdp,
				G,
				residuals,
				Cd,
				threshold,
				order,
				highorder,
				boundaryorder,
				scale,
				frequency_thin);
  printf("init: %16.9e\n", like);
  double last_like = like;

  if (!data.save_predictions("initial_predictions.txt")) {
    fprintf(stderr, "error: failed to save initial predictions\n");
  }

  //
  // Resize vectors/matrices G will be filled appropriately by likelihood
  // and will be correct size.
  //
  size_t nparam = G.cols();

  //
  // Diagonal model covariance matrices
  //
  Cm.resize(nparam, 1);
  LeastSquaresIterator::initialize_Cm(model, damping, Cm);
  
  //
  // Model vectors
  //
  model_mask.resize(nparam, 1);
  model_0.resize(nparam, 1);
  model_v.resize(nparam, 1);
  model_v_proposed.resize(nparam, 1);

  LeastSquaresIterator::copy(reference, model_0, model_mask);

  int iterations = 0;

  //
  // Apply gradient to model parameters to hopefully converge
  //
  do {

    int m = iterations % 2;
    bool valid = false;
      
    //
    // First copy parameters to model vector
    //
    do {
      LeastSquaresIterator::copy(model, model_v, model_mask);
      
      if (!step[m]->ComputeStep(epsilon[m],
				Cd,
				Cm,
				residuals,
				G,
				dLdp,
				model_mask,
				model_v,
				model_0,
				model_v_proposed)) {
      }

      valid = LeastSquaresIterator::validate(model_v_proposed, model_mask,
					     PRIOR_MIN,
					     PRIOR_MAX);
      if (!valid) {
	epsilon[m] *= 0.5;
      }
    } while (!valid);

    LeastSquaresIterator::copy(model_v_proposed, model);
    
    //
    // Recompute Likelihood
    //
    last_like = like;
    
    like = likelihood_love(data,
			   model,
			   reference,
			   damping,
			   posterior,
			   mesh,
			   love,
			   dkdp,
			   dUdp,
			   dLdp,
			   G,
			   residuals,
			   Cd,
			   threshold,
			   order,
			   highorder,
			   boundaryorder,
			   scale,
			   frequency_thin);
    if (like > last_like) {

      if (epsilon[m] < EPSILON_MIN) {
	printf("%4d: Exiting\n", iterations);
	break;
      }

      //
      // Back track and recompute (a little inefficient here)
      //
      printf("%4d: Backtracking\n", iterations);
      
      epsilon[m] *= 0.5;
      LeastSquaresIterator::copy(model_v, model);
      
      like = likelihood_love(data,
			     model,
			     reference,
			     damping,
			     posterior,
			     mesh,
			     love,
			     dkdp,
			     dUdp,
			     dLdp,
			     G,
			     residuals,
			     Cd,
			     threshold,
			     order,
			     highorder,
			     boundaryorder,
			     scale,
			     frequency_thin);
    } else {
    
      //    if (iterations % 100 == 0) {
      printf("%4d: %16.9e %16.9e\n", iterations, like, epsilon[m]);
      //    }
      
      iterations ++;
    }
    
  } while (iterations < maxiterations);
  
  return true;
}

