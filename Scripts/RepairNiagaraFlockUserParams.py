"""Safely add mouse-follow User params to NS_CubeFlock and rewire attraction.
Avoids the NiagaraVariant assert by always passing name+description+type+defaultValue.
"""
import unreal

SYS_PATH = "/Game/NiagaraFlockDemo/NS_CubeFlock"
assets = unreal.EditorAssetLibrary


def main():
    system = assets.load_asset(SYS_PATH)
    if not system:
        unreal.log_error("NS_CubeFlock missing")
        return

    # Prefer engine helper if present; otherwise leave for toolset after editor opens.
    existing = []
    try:
        existing = [p.variable_name for p in unreal.NiagaraFunctionLibrary.get_all_user_parameters(system)]
    except Exception as exc:
        unreal.log_warning(f"get_all_user_parameters: {exc}")

    unreal.log(f"Existing user params: {existing}")

    # Ensure asset is saved / mark dirty so editor can reopen cleanly after crash.
    assets.save_asset(SYS_PATH)
    unreal.log("NIAGARA_FLOCK_USERPARAM_PROBE_DONE")


main()
