using UnityEngine;
using System.Collections;
using System;
using System.Net;
using System.Net.Sockets;

public class Temp : MonoBehaviour {
	public int PORT = 7777;
	public int irows = 178;
	public int icols = 320;
	public GameObject wall;
	public float t=1f;
	public UdpClient sock;
	public IPEndPoint groupEP;
	private Texture2D tex;
	
	// Use this for initialization
	void Start () {
	
		sock = new UdpClient(PORT);
		groupEP = new IPEndPoint(IPAddress.Any, PORT);
		tex = new Texture2D(icols,irows,TextureFormat.ARGB32, false);
		for(int i=0;i<irows;i++)
		{
			for(int j=0;j<icols;j++)
			{
				tex.SetPixel(j,i,new Color32(255,255,255,255));
			}
		}
		tex.Apply();
		wall.renderer.material.mainTexture=tex;
		wall.renderer.material.mainTextureScale = new Vector2 (-1, 1);
		//Shader sh = Shader.Find("Unlit/Texture");
		Shader sh = Shader.Find("Unlit/Transparent");
		wall.renderer.material.shader=sh;
	}
	
	// Update is called once per frame
	void Update () {
	
		byte[] rec;
		rec=sock.Receive(ref groupEP);
		uint pos = 0;
		for(int i=0;i<irows;i++)
		{
			for(int j=0;j<icols;j++)
			{
				uint b=(uint)rec[pos];
				Color32 c = tex.GetPixel(j,i);
				if(b==0)
				{
					c=Color32.Lerp(c,new Color32(0,0,255,255),Time.deltaTime);
					tex.SetPixel(j,i,c);
				}
				else
				{
					c=Color32.Lerp(c,new Color32(255,0,0,0),Time.deltaTime);
					tex.SetPixel(j,i,c);
				}
				pos++;
			}
		}
		tex.Apply();
		wall.renderer.material.mainTexture=tex;
		wall.renderer.material.mainTextureScale = new Vector2 (-1, 1);
	}
	
	void onAplicationQuit(){
		sock.Close();
	}
}
