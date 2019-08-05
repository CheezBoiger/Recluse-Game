'''
  Copyright (c) 2018 Recluse Project.
  
  Compiles all shaders used by the Recluse Engine. Simply call this with no parameters.
'''
import sys, subprocess, os, shutil

path_to_shaders = 'Source'
path_to_bin = 'Bin'
spirv_binary_ext = ".spv"
compiler_path = 'glslangValidator'    # Path to the compiler of your choice. If not glslangValidator -> you may need to adjust the params for all shaders.
                                      # Along with the compile() method inside Shader.
                                      
def combine_shader_ext_with_spirv_binary_ext(filename_ext):
  return filename_ext + spirv_binary_ext

class Shader():
  def __init__(self, shader_name, shader_filename, params=''):
    '''
      shader_name : Name of the shader once compiled -> will contain the same extension as the input, with the spv appended.
      shader_filename : Name of the input shader file to be compiled.
      params : parameters for the shader when compiling. Any parameters needed must be added here, otherwise, no params as default.
    '''
    self._shader_name = shader_name
    self._shader_filename = shader_filename
    self._shader_params = params
    self._shader_ext = os.path.splitext(shader_filename)[1]
  
  def get_shader_name(self):
    return self._shader_name
    
  def get_shader_filename(self):
    return self._shader_filename
    
  def get_shader_ext(self):
    return self._shader_ext
    
  def get_shader_params(self):
    return self._shader_params
    
  def compile(self, path='.', bin='.'):
    if not os.path.isdir(bin):
      os.mkdir(bin)
    cmd = compiler_path + ' --aml -V ' + os.path.join(path, self.get_shader_filename()) + ' '
    cmd += self.get_shader_params() + ' -o '
    cmd += os.path.join(bin, self.get_shader_name() + combine_shader_ext_with_spirv_binary_ext(self.get_shader_ext()))
    print(cmd)
    subprocess.call(cmd)

'''
  Include all shaders to be compiled, here.
'''
recluse_shaders = [
  Shader('ForwardPBR_NoLR', 'ForwardPBR.frag', params='-Od'),
  Shader('ForwardPBR_NoLR_Debug', 'ForwardPBR.frag', params='-Od -DENABLE_DEBUG=1'),
  Shader('ForwardPBR_LR', 'ForwardPBR.frag', params='-Od -DLOCAL_REFLECTIONS=1'),
  Shader('ForwardPBR_LR_Debug', 'ForwardPBR.frag', params='-Od -DLOCAL_REFLECTIONS=1 -DENABLE_DEBUG=1'),
  Shader('ForwardPBR', 'ForwardPBR.vert'),
  Shader('PBR_LR_Nvidia', 'PBR.comp', params='-DNVIDIA=1 -DLOCAL_REFLECTIONS=1'),
  Shader('PBR_NoLR_Nvidia', 'PBR.comp', params='-DNVIDIA=1'),
  Shader('PBR_LR_Intel', 'PBR.comp', params='-DLOCAL_REFLECTIONS=1'),
  Shader('PBR_NoLR_Intel', 'PBR.comp'),
  Shader('PBR_LR_Amd', 'PBR.comp', params='-DAMD=1 -DLOCAL_REFLECTIONS=1'),
  Shader('PBR_NoLR_Amd', 'PBR.comp', params='-DAMD=1'),
  Shader('Sky', 'Sky.frag'),
  Shader('Sky', 'Sky.vert'),
  Shader('UI', 'UI.vert'),
  Shader('UI', 'UI.frag'),
  Shader('GBuffer', 'GBuffer.vert'),
  Shader('GBuffer', 'GBuffer.frag'),
  Shader('StaticGBuffer', 'StaticGBuffer.vert'),
  Shader('HDR', 'HDR.frag'),
  Shader('HDR', 'HDR.vert'),
  Shader('Atmosphere', 'Atmosphere.frag'),
  Shader('Atmosphere', 'Atmosphere.vert'),
  Shader('Depth', 'Depth.vert'),
  Shader('Depth', 'Depth.frag'),
  Shader('Depth_Opaque', 'Depth.frag', params='-DDEPTH_OPAQUE=1'),
  Shader('DynamicDepth', 'DynamicDepth.vert'),
  Shader('DownscaleBlurPass', 'DownscaleBlurPass.frag'),
  Shader('DownscaleBlurPass', 'DownscaleBlurPass.vert'),
  Shader('FXAA', 'FXAA.comp'),
  Shader('FXAA_Nvidia', 'FXAA.comp', params='-DNVIDIA=1'),
  Shader('FinalPass', 'FinalPass.vert'),
  Shader('FinalPass', 'FinalPass.frag'),
  Shader('RenderQuad', 'RenderQuad.vert'),
  Shader('GlowPass', 'GlowPass.frag'),
  Shader('PBR_NoLR', 'PBR.frag'),
  Shader('PBR_LR', 'PBR.frag', params='-DLOCAL_REFLECTIONS=1'),
  Shader('PBR', 'PBR.vert'),
  Shader('Particles', 'Particles.frag'),
  Shader('Particles', 'Particles.vert'),
  Shader('Particles', 'Particles.geom'),
  Shader('Particles', 'Particles.comp'),
  Shader('ForwardPBR_MorphTargets', 'ForwardPBR.vert', params='-DINCLUDE_MORPH_TARGET_ANIMATION=1'),
  Shader('DynamicDepth_MorphTargets', 'DynamicDepth.vert', params='-DINCLUDE_MORPH_TARGET_ANIMATION=1'),
  Shader('GBuffer_MorphTargets', 'GBuffer.vert', params='-DINCLUDE_MORPH_TARGET_ANIMATION=1'),
  Shader('StaticGBuffer_MorphTargets', 'StaticGBuffer.vert', params='-DINCLUDE_MORPH_TARGET_ANIMATION=1'),
  Shader('Depth_MorphTargets', 'Depth.vert', params='-DINCLUDE_MORPH_TARGET_ANIMATION=1'),
  #Shader('Depth_OpaqueMorphTargets', 'Depth.vert', params='-DINCLUDE_MORPH_TARGET_ANIMATION=1 -DDEPTH_OPAQUE=1'),
  Shader('LightClusterAssignment', 'LightClusterAssignment.comp'),
  Shader('PrefilterSpecular', 'PrefilterSpecular.comp'),
  Shader('GenerateBRDFLUT', 'GenerateBRDFLUT.comp'),
  Shader('ParticleTrail', 'ParticleTrail.vert'),
  Shader('ParticleTrail', 'ParticleTrail.geom'),
  Shader('ParticleTrail', 'ParticleTrail.frag'),
  Shader('Simple', 'Simple.frag'),
  Shader('Simple', 'Simple.vert'),
  Shader('Simple_anim', 'Simple.vert', params='-DSKIN_ANIMATION=1')
]
  
def get_shader_ext(filename):
  shader_exts = [ '.vert', '.frag', '.comp', '.geom', '.tese', '.tesc' ]
  for shader_ext in shader_exts:
    if shader_ext in filename:
      return shader_ext
  return None
 

def main(args):
  #dir = os.getcwd()
  #os.chdir(path_to_shaders)
  for shader in recluse_shaders:
    shader.compile(path_to_shaders, path_to_bin)
  #os.chdir(dir)
  return
  
if __name__ == '__main__':
  main(sys.argv)