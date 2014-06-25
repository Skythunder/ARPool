using UnityEngine;
using System.Collections;
using System;
using System.Net;
using System.Net.Sockets;


public class MaskRecSock : MonoBehaviour {
	public int PORT = 7777;
	public int irows = 178;
	public int icols = 320;
	public GameObject wall;
	public UdpClient sock;
	public IPEndPoint groupEP;
	private Texture2D tex;


	// Use this for initialization
	void Start () {


		sock = new UdpClient(PORT);
		groupEP = new IPEndPoint(IPAddress.Any, PORT);
		tex = new Texture2D(icols,irows,TextureFormat.ARGB32, false);
		Shader sh = Shader.Find("Transparent/Cutout/Diffuse");
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
				if(b==0)
					tex.SetPixel(j,i,Color.black);
				else
					tex.SetPixel(j,i,Color.clear);
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
