// Reynolds-style boids reference for Niagara Scratch Pad Custom HLSL.
// For ~2000 particles. Prefer Neighbor Grid3D for larger flocks.
//
// Suggested Scratch Pad inputs:
//   ParticleCount, NeighborRadius, SeparationWeight, AlignmentWeight,
//   CohesionWeight, MaxSpeed, FlockRadius, FollowTarget, FollowStrength
//   + Particle Attribute Readers for Positions / Velocities
// Outputs:
//   PhysicsForce, DesiredVelocity

float NearSq = NeighborRadius * NeighborRadius;
float SeparateSq = 140.0f * 140.0f;
float3 P = Particles.Position;
float3 V = Particles.Velocity;
float3 Separate = float3(0, 0, 0);
float3 AvgVel = float3(0, 0, 0);
float3 AvgPos = float3(0, 0, 0);
int Neighbors = 0;
int Self = (int)ExecutionIndex;

for (int i = 0; i < ParticleCount; ++i)
{
    if (i == Self)
    {
        continue;
    }
    bool bValid = false;
    float3 OtherP = float3(0, 0, 0);
    float3 OtherV = float3(0, 0, 0);
    Positions.GetVectorByIndex(i, bValid, OtherP);
    if (!bValid)
    {
        continue;
    }
    Velocities.GetVectorByIndex(i, bValid, OtherV);
    float3 Diff = P - OtherP;
    float D2 = dot(Diff, Diff);
    if (D2 < NearSq)
    {
        AvgVel += OtherV;
        AvgPos += OtherP;
        Neighbors++;
        if (D2 < SeparateSq)
        {
            Separate += Diff / max(D2, 1.0f);
        }
    }
}

float3 Accel = float3(0, 0, 0);
if (Neighbors > 0)
{
    float Inv = 1.0f / (float)Neighbors;
    float3 SepDir = Separate * rsqrt(max(dot(Separate, Separate), 1e-4));
    float3 AlignDir = normalize(AvgVel * Inv + 1e-4);
    float3 CohDir = normalize((AvgPos * Inv - P) + 1e-4);
    float3 VelDir = normalize(V + 1e-4);
    Accel += SeparationWeight * SepDir;
    Accel += AlignmentWeight * (AlignDir - VelDir);
    Accel += CohesionWeight * CohDir;
}

float3 Ellipsoid = P / float3(FlockRadius, FlockRadius, FlockRadius * 0.45f);
float Edge = length(Ellipsoid);
Accel -= normalize(P * float3(1, 1, 2.2) + 1e-4) * saturate((Edge - 0.7f) / 0.3f) * 4.0f;

if (FollowStrength > 0.0f)
{
    float3 ToTarget = normalize(FollowTarget - P + 1e-4);
    Accel += FollowStrength * (ToTarget - V / max(MaxSpeed, 1.0f) * 0.35f);
}

PhysicsForce = Accel * MaxSpeed * 1.8f;
DesiredVelocity = normalize(V + Accel * 0.25f + 1e-4) * MaxSpeed;
