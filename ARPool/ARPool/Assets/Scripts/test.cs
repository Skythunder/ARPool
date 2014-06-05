using UnityEngine;
using System.Collections;

public class test : MonoBehaviour {

	


float alpha = 1f;
private Material mat;

void Start ()
{
    mat = new Material(
        "Shader \"Hidden/Clear Alpha\" {" +
        "Properties { _Alpha(\"Alpha\", Float)=1.0 } " +
        "SubShader {" +
        "    Pass {" +
        "        ZTest Always Cull Off ZWrite Off" +
        "        ColorMask A" +
        "        SetTexture [_Dummy] {" +
        "            constantColor(0,0,0,[_Alpha]) combine constant }" +
        "    }" +
        "}" +
        "}"
    );
}

void OnPostRender()
{
    GL.PushMatrix();
    GL.LoadOrtho();
    mat.SetFloat( "_Alpha", alpha );
    mat.SetPass(0);
    GL.Begin( GL.QUADS );
    GL.Vertex3( 0f, 0f, 0.1f );
    GL.Vertex3( 1f, 0f, 0.1f );
    GL.Vertex3( 1f, 1f, 0.1f );
    GL.Vertex3( 0f, 1f, 0.1f );
    GL.End();
    GL.PopMatrix();

}
}