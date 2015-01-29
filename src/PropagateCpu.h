#pragma once

#include "Propagate.h"

class PropagateCpu : public Propagate {
public:
    PropagateCpu( OpenCLHelper *cl, LayerDimensions dim, ActivationFunction const*fn ) :
            Propagate( cl, dim, fn )
        {
    }
    virtual void propagate( int batchSize, CLWrapper *inputDataWrapper, CLWrapper *weightsWrapper, CLWrapper *biasWeightsWrapper, CLWrapper *resultsWrapper ) {
        inputDataWrapper->copyToHost();
        weightsWrapper->copyToHost();
    //    weightsWrapper->copyToHost();
      //  biasWeightsWrapper->copyToHost();
        float *biasWeights = 0;
        if( dim.biased ) {
            biasWeightsWrapper->copyToHost();
            biasWeights =  (float *)biasWeightsWrapper->getHostArray();
        }
        float *results = propagate( batchSize, (float *)inputDataWrapper->getHostArray(), (float *)weightsWrapper->getHostArray(), biasWeights );
        int resultsSize = batchSize * dim.outputCubeSize;
//        memcpy( (float *)resultsWrapper->getHostArray(), results, sizeof(float) * resultsSize );
        float *hostArray = (float *)resultsWrapper->getHostArray();
        for( int i = 0; i < resultsSize; i++ ) {
            hostArray[i] = results[i];
        }
        resultsWrapper->copyToDevice();
        delete[] results;
    }
    virtual float *propagate( int batchSize, float *inputData, float *weights, float *biasWeights ) {
        float *results = new float[ dim.outputCubeSize * batchSize ];
        for( int n = 0; n < batchSize; n++ ) {
            for( int filter = 0; filter < dim.numFilters; filter++ ) {
                for( int outRow = 0; outRow < dim.outputBoardSize; outRow += 1 + dim.skip ) {
                    for( int outCol = 0; outCol < dim.outputBoardSize; outCol += 1 + dim.skip ) {
                        float sum = 0;
                        for( int inPlane = 0; inPlane < dim.inputPlanes; inPlane++ ) {
                            for( int u = -dim.halfFilterSize; u <= dim.halfFilterSize; u++ ) {
                                int inRow = outRow * ( dim.skip + 1 ) + u + ( dim.padZeros ? 0 : dim.halfFilterSize );
//                                cout << "candidate inRow " << inRow << endl;
                                if( inRow < 0 || inRow > dim.inputBoardSize - 1 ) {
                                    continue;
                                }
                                int filterRow = u + dim.halfFilterSize;
                                for( int v = -dim.halfFilterSize; v <= dim.halfFilterSize; v++ ) {
                                    int inCol = outCol * ( dim.skip + 1 ) + v + ( dim.padZeros ? 0 : dim.halfFilterSize );
                                    int filterCol = v + dim.halfFilterSize;
                                    if( inCol < 0 || inCol > dim.inputBoardSize - 1 ) {
                                        continue;
                                    }
                                    int inputIndex = ( ( n * dim.inputPlanes + inPlane )
                                        * dim.inputBoardSize + inRow )
                                        * dim.inputBoardSize + inCol;
                                    int weightIndex = ( ( filter 
                                        * dim.inputPlanes + inPlane ) 
                                        * dim.filterSize  + filterRow )
                                        * dim.filterSize  + filterCol;
//                                    cout << "inpos " << inRow << "," << inCol << " outpos " << outRow << "," << outCol
//                                        << " filterpos " << filterRow << "," << filterCol << endl;
                                    float sumchange = inputData[ inputIndex] * weights[ weightIndex ];
                                    if( sumchange != 0 ) {
//                                        cout << inputData[inputIndex] << " * " << weights[weightIndex] << " = " << sumchange << endl;
                                    }
                                    sum += sumchange;
                                }
                            }
                        }
                        if( dim.biased ) {
                            sum += biasWeights[filter];
                        }
                        sum = fn->calc( sum );
                        int resultsIndex = ( ( n * dim.numFilters + filter ) 
                            * dim.outputBoardSize + outRow )
                            * dim.outputBoardSize + outCol;
                        results[resultsIndex] = sum;
                    }
                }
            }
        }
        return results;
    }
};

