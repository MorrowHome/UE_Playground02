#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphResources.h"

class FMassSpringTestCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FMassSpringTestCS);
	SHADER_USE_PARAMETER_STRUCT(FMassSpringTestCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER(uint32, ValueCount)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float4>, OutValues)
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_GLOBAL_SHADER(FMassSpringTestCS,
	"/Plugin/MassSpring/Private/MassSpring.usf", "TestCS", SF_Compute);