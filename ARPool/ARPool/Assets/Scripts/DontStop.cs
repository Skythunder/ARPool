using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System;
using System.Net;
using System.Net.Sockets;

public class DontStop : MonoBehaviour {
	public const int PORT = 7777;
	public const int irows = 178;
	public const int icols = 320;
	public const int SPAWN = 1;
	public double score = 0;
	public int lives = 3;
	public GameObject ef;
	public GameObject movef;
	public GameObject bomb;
	public GUIText guit;
	public GUIText guil;
	public GameObject tester;
	private UdpClient sock;
	private IPEndPoint groupEP;
	private ArrayList efl;
	private Dictionary<String,bool> isMonster;
	private int nbombs;
	private ArrayList blist;
	
	GameObject spawnBomb(){
		GameObject g;
		Vector3 pos;
		bool repeat;
		do
		{
			repeat=true;
			float ry = (float)UnityEngine.Random.Range(-irows/2,irows/2);
			float rx = (float)UnityEngine.Random.Range(-icols/2,icols/2);
			pos = new Vector3(rx,ry,4f);
			g = GameObject.Find(pos.ToString());
			if(!g)
			{
				repeat = false;
				GameObject ob = (GameObject)Instantiate(tester,pos,Quaternion.identity);
				Collider[] hits = Physics.OverlapSphere(pos,ob.GetComponent<SphereCollider>().radius);
				GameObject.Destroy(ob);
				if(hits.Length>1)
					repeat=true;
				//Debug.Log(hits.Length);
			}
		}
		while(repeat);
		GameObject b =(GameObject)Instantiate(bomb,pos,Quaternion.identity);
		b.name=pos.ToString();
		return b;
	}

	// Use this for initialization
	void Start () {
	
		Screen.SetResolution(1400,1050,true);
		sock = new UdpClient(PORT);
		groupEP = new IPEndPoint(IPAddress.Any, PORT);
		efl = new ArrayList();
		isMonster = new Dictionary<String,bool>();
		nbombs = 3;
		blist = new ArrayList();
		blist.Add(spawnBomb());
		blist.Add(spawnBomb());
		blist.Add(spawnBomb());
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
			Vector3 wp = new Vector3(floatArr[i]-(icols/2),floatArr[i+1]-(irows/2),4f);
			
			GameObject go; 
			go = GameObject.Find(wp.ToString());
			if(!go)
			{
				go=(GameObject)Instantiate(movef,wp,Quaternion.identity);
				go.transform.localScale+=new Vector3(5,5,5);
				Collider[] hits = Physics.OverlapSphere(wp,go.GetComponent<SphereCollider>().radius);
				if(hits.Length>1)
				{
					foreach(Collider c in hits)
					{
						if(c.gameObject.tag=="bomb")
						{
							lives--;
							GameObject.Destroy(c.gameObject);
							//if(lives<=0)
								
						}
					}
				}
				
			}
			else
			{
				if(isMonster.ContainsKey(wp.ToString()))
				{
					int raux = UnityEngine.Random.Range(1,101);
					if(raux>=(100-SPAWN))
					{
						isMonster[wp.ToString()]=true;
						GameObject.Destroy(go);
						go=(GameObject)Instantiate(ef,wp,Quaternion.identity);
					}
				}
				else
					isMonster.Add(wp.ToString(),false);
					
			}
			temp.Add(go);
			go.name=wp.ToString();
		}
		foreach(GameObject g in efl)
			GameObject.Destroy(g);
		efl.Clear();
		efl=temp;
		var buffer = new List<String>(isMonster.Keys);
		foreach(String x in buffer)
		{
			GameObject g = GameObject.Find(x);
			if(g)
			{
				if(isMonster[x])
					score-=1*Time.deltaTime;
			}
			else
			{
				if(isMonster[x])
					score+=10;
				isMonster.Remove(x);
			}
		}
		buffer.Clear();
		guit.text="Score: "+(int)score;
		guil.text="Lives: "+lives;
		if((score/100)>nbombs)
		{
			spawnBomb();
			nbombs++;
		}
		
	}
	
	void onAplicationQuit(){
		sock.Close();
	}
	
}
