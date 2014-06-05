using UnityEngine;
using System.Collections;
using System;
using System.Net;
using System.Net.Sockets;

public class SocketCom : MonoBehaviour {
	public const int PORT = 7777;
	public UdpClient sock;
	public IPEndPoint groupEP;
	public ArrayList efl;
	public GameObject ef;
	public const int irows = 178;
	public const int icols = 320;
		
	public GameObject[] particles;
	
	// Use this for initialization
	void Start () {
		//Screen.SetResolution(1400,1050,true);
		sock = new UdpClient(PORT);
		groupEP = new IPEndPoint(IPAddress.Any, PORT);
		efl = new ArrayList();
	}
	
	// Update is called once per frame
	void Update () {
		byte[] rec;
		rec=sock.Receive(ref groupEP);
		/*foreach(GameObject x in efl)
		{
			GameObject.Destroy(x);
		}
		efl.Clear();*/
		float[] floatArr = new float[rec.Length / 4];
		for (int i = 0; i < floatArr.Length; i++) 
		{
		    floatArr[i] = BitConverter.ToSingle(rec, i * 4);
		}
		ArrayList temp = new ArrayList();
		int j=0;
		for(int i=0;i<floatArr.Length;i+=2)
		{
			Vector3 wp = new Vector3(floatArr[i]-(icols/2),0f,floatArr[i+1]-(irows/2));
			GameObject go; 
			go = GameObject.Find(wp.ToString());
			if(!go)
			{
				if(j<particles.Length)
				{
					go=(GameObject)particles[j];
					go.transform.position=wp;
					/*float dist = Mathf.Infinity;
					int pos=-1;
					for(int x=0;x<particles.Length;x++)
					{
						float dif=(particles[x].transform.position-wp).sqrMagnitude;
						if(dif<dist)
						{
							dist=dif;
							pos=x;
						}
					}
					go=(GameObject)particles[pos];
					go.transform.position=wp;*/
				}
				else
				{
					go = (GameObject)Instantiate(ef,wp,Quaternion.identity);
					go.transform.localScale+=new Vector3(10,10,10);
				}
				go.name=wp.ToString();
				
			}
			temp.Add(go);
			j++;
		}
		ArrayList aux = new ArrayList(particles);
		foreach(GameObject x in efl)
		{
			if(!aux.Contains(x))
			{
				if(!temp.Contains(x))
				{
					GameObject.Destroy(x);
				}
			}
		}
		efl=temp;
	}
	
	void onAplicationQuit(){
		sock.Close();
	}
}
