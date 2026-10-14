"""Compile the product's portable core and verify it against independent matrices."""
import json
import os
import shutil
import subprocess
from pathlib import Path

import numpy as np
from scipy.spatial.transform import Rotation

ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'build-tests'; out.mkdir(exist_ok=True)
compiler=shutil.which('g++')
if not compiler: raise SystemExit('A host g++ compiler is required for these tests.')
exe=out/'core_tests.exe'
subprocess.run([compiler,'-std=c++20','-O2','-I'+str(ROOT/'include'),'-I'+str(ROOT/'extern/includes'),
                str(ROOT/'tests/core_tests.cpp'),'-o',str(exe)],check=True,cwd=ROOT)
subprocess.run([str(exe)],check=True)
data=subprocess.check_output([str(exe),'--sweep'],text=True)
def m(p,e):
    result=np.eye(4); result[:3,:3]=Rotation.from_euler('zxy',[e[2],e[0],e[1]],degrees=True).as_matrix(); result[:3,3]=p
    return result
bridge=np.eye(4)
bridge[:3,:3]=Rotation.from_quat([-.5,0,0,float(np.float32(.8660253882408142))]).as_matrix()
bridge[:3,3]=[0,float(np.float32(-.03)),float(np.float32(-.04))]
unit_cases=0
for line in subprocess.check_output([str(exe),'--units'],text=True).splitlines():
    cm,bits=line.split(',')
    expected=np.float32(np.float32(float(cm))*np.float32(.01))
    assert int(bits)==int(expected.view(np.uint32))
    unit_cases+=1
max_pos,max_rot=0.,0.
for line in data.splitlines():
    v=[float(x) for x in line.split(',')]
    raw=m([.1,.9,-.4],v[:3]); off=m([0,0,0],v[3:6]); off[:3,3]=[-.02,.006,-.04]+off[:3,:3]@np.array([0,0,.155])
    anchor=m([-.005,.03,.055],v[6:9]); expected=raw@bridge@off@np.linalg.inv(anchor)
    actual=m(v[9:12],[0,0,0]); actual[:3,:3]=Rotation.from_quat(v[12:16]).as_matrix()
    np.testing.assert_allclose(actual,expected,atol=3e-12,rtol=0)
    np.testing.assert_allclose(m([0,0,0],v[16:19])[:3,:3],expected[:3,:3],atol=3e-12,rtol=0)
    # Final anchor pose must equal PC-style raw * basis * EasyOffset.
    np.testing.assert_allclose(actual@anchor,raw@bridge@off,atol=3e-12,rtol=0)
    max_pos=max(max_pos,float(np.linalg.norm(actual[:3,3]-expected[:3,3])*1000))
    max_rot=max(max_rot,float(np.degrees(Rotation.from_matrix(actual[:3,:3].T@expected[:3,:3]).magnitude())))
for path in (ROOT/'sample-profiles').glob('*.json'):
    returned=json.loads(subprocess.check_output([str(exe),str(path)],text=True))
    original=json.loads(path.read_text())
    assert returned.keys()==original.keys()
    for hand in ['Left','Right']:
        for suffix in ['SaberPivotPosition','SaberRotationEuler','SaberZOffset','SaberReference','SaberHasReference']:
            before,after=original[hand+suffix],returned[hand+suffix]
            if isinstance(before,dict):
                np.testing.assert_allclose(list(before.values()),list(after.values()),atol=1e-12,rtol=0)
            elif isinstance(before,bool): assert after==before
            else: np.testing.assert_allclose(after,before,atol=1e-12,rtol=0)
report=dict(caseCount=2000,matrixMaximumPositionErrorMillimetres=max_pos,matrixMaximumRotationErrorDegrees=max_rot,
            profileValidationPassed=True,sampleProfilesRoundTripPassed=True,pcFloatUnitConversionCases=unit_cases,
            sourceBridgeFloatBitsVerified=True,gameRuntimeTested=False)
runtime_exe=out/'runtime_pose_tests.exe'
subprocess.run([compiler,'-std=c++20','-O2','-ffp-contract=off','-I'+str(ROOT/'tests/stubs'),'-I'+str(ROOT/'include'),
                str(ROOT/'tests/runtime_pose_tests.cpp'),'-o',str(runtime_exe)],check=True,cwd=ROOT)
runtime_data=subprocess.check_output([str(runtime_exe)],text=True)
runtime_pos,runtime_rot=0.,0.
def pose_matrix(row):
    a=np.eye(4);a[:3,3]=row[:3];a[:3,:3]=Rotation.from_quat(row[3:7]).as_matrix();return a
for line in runtime_data.splitlines():
    v=[float(x) for x in line.rstrip(',').split(',')]
    raw,off,parent,child,relative,final=[pose_matrix(v[i:i+7]) for i in range(0,42,7)]
    expected_relative=parent@child
    expected=raw@bridge@off@np.linalg.inv(expected_relative)
    delta=np.linalg.inv(expected)@final
    runtime_pos=max(runtime_pos,float(np.linalg.norm(delta[:3,3])*1000))
    runtime_rot=max(runtime_rot,float(np.degrees(Rotation.from_matrix(delta[:3,:3]).magnitude())))
    np.testing.assert_allclose(relative,expected_relative,atol=7e-7,rtol=0)
assert runtime_pos<.003 and runtime_rot<.0001
report.update(runtimeFloatModelCaseCount=2000,runtimeFloatModelMaximumPositionErrorMillimetres=runtime_pos,
              runtimeFloatModelMaximumRotationErrorDegrees=runtime_rot,localAnchorIndependentOfRootWorldPose=True,
              invalidAnchorHierarchyRejected=True,runtimeFloatModelUsesHostUnityStandIns=True)
(out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
