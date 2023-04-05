extends Node2D

func _ready():
	var mp = $ModPlayer
	mp.filename = "res://bananasplit.mod"
	mp.load()
	mp.play()

func _unhandled_key_input(event):
	if event is InputEventKey and event.pressed:
		if event.scancode == KEY_1:
			$ModPlayer.tempo = 125
		elif event.scancode == KEY_2:
			$ModPlayer.tempo = 150
