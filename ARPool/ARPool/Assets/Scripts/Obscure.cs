using UnityEngine;
using System.Collections;

public class Obscure : MonoBehaviour {

	// Use this for initialization
	void Start () {
		Component[] rl=GetComponentsInChildren<Renderer>();
		foreach(Renderer r in rl)
		{
			r.material.renderQueue = 2002;
		}
	}
	
	// Update is called once per frame
	void Update () {
	
	}
}
