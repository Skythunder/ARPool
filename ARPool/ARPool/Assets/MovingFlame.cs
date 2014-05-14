using UnityEngine;
using System.Collections;

public class MovingFlame : MonoBehaviour {
	public GameObject flame;
	public float speed=5;
	public float xt=15f;
	public float yt=15f;
	private bool right;
	public GameObject wall;
	
	// Use this for initialization
	void Start () {
		right=true;
		
	}
	
	// Update is called once per frame
	void Update () {
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
