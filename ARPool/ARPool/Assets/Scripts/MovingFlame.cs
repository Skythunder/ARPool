using UnityEngine;
using System.Collections;
using System;
using System.Net;
using System.Net.Sockets;

public class MovingFlame : MonoBehaviour {
	public const int PORT = 7777;
	public UdpClient sock;
	public IPEndPoint groupEP;
	private ArrayList efl;
	public GameObject flame;
	public float speed=5;
	public float xt=15f;
	public float yt=15f;
	private bool right;
	
	// Use this for initialization
	void Start () {
		right=true;
		Screen.SetResolution(1400,1050,true);
		sock = new UdpClient(PORT);
		groupEP = new IPEndPoint(IPAddress.Any, PORT);
		efl = new ArrayList();
	}
	
	// Update is called once per frame
	void Update () {
		byte[] rec;
		rec=sock.Receive(ref groupEP);
		
		float[] floatArr = new float[rec.Length / 4];
		for (int i = 0; i < floatArr.Length; i++) 
		{
		    floatArr[i] = BitConverter.ToSingle(rec, i * 4);
		}
		ArrayList temp = new ArrayList();
		for(int i=0;i<floatArr.Length;i+=2)
		{
			Vector3 wp = new Vector3(floatArr[i],floatArr[i+1],2f);
			GameObject go; 
			go = GameObject.Find(wp.ToString());
			if(!go)
			{
				go=(GameObject)Instantiate(flame,wp,Quaternion.identity);
			}
			temp.Add(go);
			go.name=wp.ToString();
		}
		foreach(GameObject g in efl)
			GameObject.Destroy(g);
		efl.Clear();
		efl=temp;
		
		///
		Vector3 pos;
		pos=flame.transform.position;
		if(pos.x>=xt&&right)
		{
			right=false;
		}
		if(pos.x<=-xt&&!right)
		{
			right=true;
		}
		if(right)
			flame.transform.Translate(Vector3.right*speed*Time.deltaTime);
		else
			flame.transform.Translate(-Vector3.right*speed*Time.deltaTime);
		pos=flame.transform.position;
		
	}
}
