extends Node2D

func _ready():
	var mp = $ModPlayer
	mp.filename = "/home/furrykef/Downloads/bananasplit.mod"
	mp.load()
	mp.play()
