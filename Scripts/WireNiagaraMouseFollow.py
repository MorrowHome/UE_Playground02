"""Wire NS_CubeFlock mouse follow via Niagara Toolset APIs.

IMPORTANT: NiagaraExt_UserVariable requires ALL of:
  name, description, type, defaultValue
Missing `type` caused NiagaraVariant.cpp assert (InCount > 0) and editor crash.
"""
import unreal

# This script is meant to be invoked from an editor Python console / commandlet
# AFTER the CubeFlock plugin is compiled. It uses only high-level unreal APIs
# that are known-safe; Toolset AddUserVariables is done from MCP with full fields.

SYS = "/Game/NiagaraFlockDemo/NS_CubeFlock"


def main():
    system = unreal.EditorAssetLibrary.load_asset(SYS)
    assert system, SYS
    try:
        infos = unreal.NiagaraFunctionLibrary.get_all_user_parameters(system)
        names = [str(i.variable_name) for i in infos]
    except Exception as exc:
        names = []
        unreal.log_warning(str(exc))
    unreal.log("USER_PARAMS " + ",".join(names))
    unreal.EditorAssetLibrary.save_asset(SYS)
    unreal.log("WIRE_MOUSE_FOLLOW_PROBE_OK")


main()
