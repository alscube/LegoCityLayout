"""Render orthographic switch artwork from the attributed LDraw assemblies.

LDraw: 20 units per stud. Canvas: 24 x 36 studs; entrance at y=1.
The main line is x=4 (right switch) or x=20 (left switch).
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter
ROOT = Path(__file__).resolve().parents[1]
LIB = ROOT / 'resources' / 'ldraw-switches'
cache = {}

def load(name):
    name = name.lower().replace('\\', '/')
    if name in cache: return cache[name]
    paths = [LIB/'parts'/name, LIB/'p'/name]
    p = next((p for p in paths if p.exists()), None)
    if p is None: raise FileNotFoundError(name)
    faces = []
    for line in p.read_text(errors='replace').splitlines():
        v=line.split()
        if not v or v[0] not in ['1','3','4']: continue
        c=int(v[1]); nums=list(map(float, v[2:14] if v[0]=='1' else v[2:]))
        if v[0]=='1':
            t=np.array(nums[:3]); m=np.array(nums[3:]).reshape(3,3)
            child=' '.join(v[14:]).lower().replace('\\', '/')
            if child == 's/53401s02.dat':
                # Match the stock straight/curve connector handedness and the
                # physical switch: swap tongue and socket across the end sleeper.
                # Reflect only the connector submesh, retaining each seam origin.
                m=m @ np.diag([1.0,1.0,-1.0])
            for subc, pts, is_stud in load(child):
                faces.append((c if subc==16 else subc, pts @ m.T + t, is_stud or name == "stud.dat"))
        else:
            pts=np.array(nums).reshape(-1,3)
            faces.append((c,pts[:3],name == "stud.dat"))
            if len(pts)==4: faces.append((c,pts[[0,2,3]],name == "stud.dat"))
    cache[name]=faces
    return faces

def render(name, output, left=False, canvas=(24,36), origin=None, horizontal=False):
    faces=load(name)
    print(name, len(faces), 'triangles')
    points=np.concatenate([p for _,p,_ in faces]);print('model bounds',points.min(0),points.max(0))
    ss=2; pps=40; scale=pps*ss/20
    w,h=canvas[0]*pps*ss,canvas[1]*pps*ss
    if origin is None: origin=(20 if left else 4,17)
    depth=np.full((h,w),np.inf,dtype=np.float32)
    rgb=np.zeros((h,w,3),dtype=np.uint8)
    studs=np.zeros((h,w),dtype=bool)
    plastic=np.zeros((h,w),dtype=bool)
    light=np.array([-.4,-1,-.55]);light/=np.linalg.norm(light)
    for color, p, is_stud in faces:
        if horizontal:
            xy=np.column_stack((p[:,0]/20+origin[0],-p[:,2]/20+origin[1]))*pps*ss
        else:
            xy=np.column_stack((-p[:,2]/20+origin[0],p[:,0]/20+origin[1]))*pps*ss
        lo=np.maximum(np.floor(xy.min(0)).astype(int),0)
        hi=np.minimum(np.ceil(xy.max(0)).astype(int),[w-1,h-1])
        if np.any(hi<lo):continue
        a,b,c=xy
        den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        if abs(den)<1e-8:continue
        yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1];xx=xx+.5;yy=yy+.5
        u=((b[1]-c[1])*(xx-c[0])+(c[0]-b[0])*(yy-c[1]))/den
        v=((c[1]-a[1])*(xx-c[0])+(a[0]-c[0])*(yy-c[1]))/den
        z=u*p[0,1]+v*p[1,1]+(1-u-v)*p[2,1]
        view=depth[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
        mask=(u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7)&(z<view)
        if not mask.any():continue
        n=np.cross(p[1]-p[0],p[2]-p[0]);n/=np.linalg.norm(n)
        if n[1]>0:n=-n
        strength=.57+.43*max(0,float(n@light))
        base=np.array([235,185,34]) if color==14 else np.array([106,112,116])
        rgb[lo[1]:hi[1]+1,lo[0]:hi[0]+1][mask]=np.clip(base*strength,0,255).astype(np.uint8)
        view[mask]=z[mask]
        studs[lo[1]:hi[1]+1,lo[0]:hi[0]+1][mask]=is_stud
        plastic[lo[1]:hi[1]+1,lo[0]:hi[0]+1][mask]=color != 14
    valid=np.isfinite(depth)
    # Gentle contact shading around raised rails and studs.
    heights=np.where(valid,np.clip(24-depth,0,60),0).astype(np.uint8)
    blurred=np.asarray(Image.fromarray(heights).filter(ImageFilter.GaussianBlur(3*ss)))
    shade=1-np.clip((blurred.astype(float)-heights)/80,0,.25)
    raised=np.where(valid,np.clip(-depth,0,8),0)*1.8
    gray=plastic
    rgb=np.clip(rgb*shade[:,:,None] + (raised*gray)[:,:,None],0,255).astype(np.uint8)
    # Catch the light along raised rail edges and molded sleeper edges, as
    # on the photographic curved-track artwork. Keep the underlying gray dark.
    surface=np.where(valid,np.clip(8-depth,0,16),8).astype(np.uint8)
    smooth_surface=np.asarray(Image.fromarray(surface).filter(ImageFilter.GaussianBlur(ss)),dtype=float)
    surface_y,surface_x=np.gradient(smooth_surface)
    edge_light=surface_x+surface_y
    alpha_soft=np.asarray(Image.fromarray(valid.astype(np.uint8)*255).filter(ImageFilter.GaussianBlur(ss*.6)),dtype=float)/255
    alpha_y,alpha_x=np.gradient(alpha_soft)
    rail_highlight=np.maximum(edge_light,0)*36+np.minimum(edge_light,0)*10
    body_highlight=np.maximum(alpha_x+alpha_y,0)*100
    highlight=(rail_highlight+body_highlight)*gray*valid*~studs
    rgb=np.clip(rgb.astype(float)+highlight[:,:,None],0,255).astype(np.uint8)
    # Top views hide cylindrical sidewalls. Model the stud rim and a short
    # directional shadow so the raised studs remain legible in the palette.
    stud_image=Image.fromarray(studs.astype(np.uint8)*255)
    softened=np.asarray(stud_image.filter(ImageFilter.GaussianBlur(ss)),dtype=float)/255
    grad_y,grad_x=np.gradient(softened)
    rim=studs & (np.asarray(stud_image.filter(ImageFilter.MinFilter(2*ss+1))) < 255)
    stud_edge=grad_x+grad_y
    relief=studs*14.0-rim*8.0+(np.maximum(stud_edge,0)*350+np.minimum(stud_edge,0)*220)*studs
    shadow=np.zeros((h,w),dtype=float)
    shift=3*ss
    shadow[shift:,shift:]=softened[:-shift,:-shift]
    shadow*=valid & ~studs & gray
    rgb=np.clip(rgb.astype(float)*(1-shadow[:,:,None]*.28)+relief[:,:,None],0,255).astype(np.uint8)
    # The application matches curved-track plastic to the cropped straight
    # photo. Use that same gray reference while preserving the rendered relief.
    reference=np.asarray(Image.open(ROOT/'resources'/'StrightTrack.png').convert('RGBA'))
    if reference.shape[:2] == (1142,1377): reference=np.rot90(reference)
    reference=reference[:1344,310:950]
    target=reference[:,:,:3][reference[:,:,3]>=240].mean(axis=0)
    visible_plastic=plastic & valid
    source=rgb[visible_plastic].mean(axis=0)
    rgb[visible_plastic]=np.clip(np.rint(rgb[visible_plastic].astype(float)+target-source),0,255).astype(np.uint8)
    # A narrow, pale reflection follows the elevated railheads, including
    # the diverging route and switch blade. Studs and sleepers are excluded.
    railheads=valid & plastic & ~studs & (depth < -14.5)
    rail_soft=np.asarray(Image.fromarray(railheads.astype(np.uint8)*255).filter(ImageFilter.GaussianBlur(ss*.6)),dtype=float)/255
    rail_y,rail_x=np.gradient(rail_soft)
    reflection=np.clip((rail_x+rail_y)*3.0,0,.8)*railheads*.5
    white=np.array([224,228,232],dtype=float)
    rgb=np.clip(rgb.astype(float)*(1-reflection[:,:,None])+white*reflection[:,:,None],0,255).astype(np.uint8)
    rgba=np.dstack((rgb,valid.astype(np.uint8)*255))
    Image.fromarray(rgba).resize((canvas[0]*pps,canvas[1]*pps),Image.Resampling.LANCZOS).save(output)

if __name__=='__main__':
    right_output = ROOT/'resources/RightSwitchTrack.png'
    render('53404c01.dat', right_output)
    with Image.open(right_output) as artwork:
        rotated = artwork.transpose(Image.Transpose.ROTATE_90)
    rotated.save(right_output)
    left_output = ROOT/'resources/LeftSwitchTrack.png'
    render('53407c01.dat', left_output, left=True)
    with Image.open(left_output) as artwork:
        rotated = artwork.transpose(Image.Transpose.ROTATE_270)
    rotated.save(left_output)
