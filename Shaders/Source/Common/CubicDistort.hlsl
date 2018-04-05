/*
        Cubic Lens Distortion HLSL Shader
       
        Original Lens Distortion Algorithm from SSontech (Syntheyes)
        http://www.ssontech.com/content/lensalg.htm
       
        r2 = image_aspect*image_aspect*u*u + v*v
        f = 1 + r2*(k + kcube*sqrt(r2))
        u' = f*u
        v' = f*v
 
        author : François Tarlier
        website : www.francois-tarlier.com/blog/index.php/2009/11/cubic-lens-distortion-shader
 
*/
 
 
sampler s0 : register(s0);
 
float4 main(float2 tex : TEXCOORD0) : COLOR
{
       
        // lens distortion coefficient (between
        float k = -0.15;
       
        // cubic distortion value
        float kcube = 0.5;
       
       
        float r2 = (tex.x-0.5) * (tex.x-0.5) + (tex.y-0.5) * (tex.y-0.5);       
        float f = 0;
       
 
        //only compute the cubic distortion if necessary
        if( kcube == 0.0){
                f = 1 + r2 * k;
        }else{
                f = 1 + r2 * (k + kcube * sqrt(r2));
        };
       
        // get the right pixel for the current position
        float x = f*(tex.x-0.5)+0.5;
        float y = f*(tex.y-0.5)+0.5;
        float3 inputDistord = tex2D(s0,float2(x,y));
 
 
        return float4(inputDistord.r,inputDistord.g,inputDistord.b,1);
}