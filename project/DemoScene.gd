extends Node2D

func _ready():
	var mp = $ModPlayer
	mp.filename = "res://bananasplit.mod"
	mp.load()
	mp.play()
