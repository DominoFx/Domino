s.meter;
Server.killAll;

//to execute, doucle click just inside the parenthesis to select all and press shift-enter


(
fork {
	Server.local.options.numOutputBusChannels = 8;
	// Server.local.options.numInputBusChannels = 0;
	Server.local.options.device = "USB Sound Device"; //change name of soundcard here
	// Server.local.options.device = "Scarlett 6i6 USB";
	// Server.local.options.device.postln;
	s.bootSync;
	s.waitForBoot;
	s.sync;

	//no need to use this for this test script
/*	~numSpeakers = 6; //change # of Speakers here
	~numDominos = 12; // change # of dominos used here
	~sDiv = 1/(~numSpeakers-1);*/

	///sounds buffers///
	~marimba = "/home/pi/Domino_ARCHIVE/testComplet_1dom_4juin/sounds/voc_sing/*.aif".resolveRelative.pathMatch; //change sound folder here for other sounds
	~numBufsMar = ~marimba.size;
	~bufs = Array.newClear(~numBufsMar);
	~marimba.do({arg item, i;
		~bufs[i] = Buffer.read(s, item);
	});
	s.sync;
/*	~bufs.postln;*/

	///variables///
	~scratch = true;
	~collGate = true;
	~falling = false;
	~speed = 0;
	~angle = 1;

	/*a = SynthDef(\play_sound, {| out = 0, bufnum = 0, mul1 = 0, mul2 = 0, mul3 = 0, mul4 = 0 |
	Out.ar(out,
	[PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), doneAction:2)*mul1,
	PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), doneAction:2)*mul1,
	PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), doneAction:2)*mul2,
	PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), doneAction:2)*mul2,
	PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), doneAction:2)*mul3,
	PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum), doneAction:2)*mul3
	];
	);
	}).send;
	s.sync;
	a.play;
	a.postln;*/

	///audio controls///
	~pitch_bus = Bus.control();
	~gain_bus = Bus.control();

	~pitch_synth = SynthDef(\pitch_s, {
		| outBus = 10, pitch = 1 |
		Out.kr(outBus,
			Line.kr(1, 1, 10, pitch, 0)
		)
	}, [0, 0.2]).play(s, [\outBus, ~pitch_bus, \pitch, 1]);

	~gain_synth = SynthDef(\gain_s, {
		| outBus = 11, gain = 1 |
		Out.kr(outBus,
			Line.kr(1, 1, 10, gain, 0)
		)
	}, [0, 0.2]).play(s, [\outBus, ~gain_bus, \pitch, 1]);

	~gain_bus.set(0);
	~speed_bus.set(0);


	/*	a = SynthDef(\scratch, {| out = 0, bufnum = 0, pitch = 3, mul = 1 |
	Out.ar(out,
	PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum)*pitch, loop: 1.0)*mul
	)}).play(s, [\out, 0, \bufnum, b, \pitch, 1, \mul, 1]);


	b = SynthDef(\coll, {| out = 0, bufnum = 0, pitch = 3, mul = 1 |
	Out.ar(out,
	PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum)*pitch, loop: 1.0)*mul
	)}).play(s, [\out, 0, \bufnum, b, \pitch, 1, \mul, 1]);*/


	SynthDef(\dom, {| out = 0, bufnum = 0, gate = 1, pitch = 1, mul = 1, release = 0.02, loop = 0 |
		var env = Env.adsr(0.02, releaseTime: release);
		var gen = EnvGen.kr(env, gate, doneAction: 2);
		Out.ar(out,
			PlayBuf.ar(1, bufnum, BufRateScale.kr(bufnum)*pitch, loop: loop)*mul*gen
	)}).add;


	~scratching = Routine({//to print
		"scratching".postln;
		b = Synth(\dom, [\gate, 1, \pitch, 1, \release, 0.01, \loop, 1]);
		~gain_synth.set(\gain, 0);
		b.map(\pitch, ~pitch_bus, \mul, ~gain_bus);
		while({~scratch},
			{
				// "insideWhile".postln;
				if ((((~speed <= 5) && (~speed >= -5)) && ((~angle < 0.1) && (~angle > -0.1))),
					{
						/*"insidewhile".postln;
						"speed".post;~speed.postln;
						"angle".post;~angle.postln;*/
						// 0.02.wait;//?
						~collGate = true;
						// "collGateIsOn".postln;
						0.05.wait;
					},
					{
						~collGate = false;
						// "collGateFalse".postln;
						0.05.wait;
				});
		});
	});


	~afterColl = Routine({//to print
		"afterColl".postln;
		0.05.wait;
		while({~falling},
			{
				// "insideWhileAfterColl".postln;
				if ((~speed >= 5) || (~speed <= -5),
					{
						~scratch = false;
						0.05.wait;
					},
					{
						~falling = false;
						~scratch = true;
						"scratchShouldBeOn".postln;
						~scratching.reset;
						~scratching.play;
						0.05.wait;

				});
		});
	});


	///OSC///
	o= OSCdef.new(
		\accel,
		{
			arg msg,time, addr, port;
			var current;
			current = msg[1];
			 msg.postln;
			// msg.size.postln;
			if (msg.size == 2,
				{
					"collGate".post;~collGate.postln;
					if (~collGate,
						{
							b.release;
							~scratch = false;
							c = Synth(\dom, [\gate, 1, \pitch, 1, \release, 0.02, \loop, 0, \bufnum, ~bufs[current]]);
							~collGate = false;
							/*"receivedColl".postln;
							"collGate".post;~collGate.postln;*/
							~falling = true;
							~afterColl.reset;
							~afterColl.play;
							// ~coll = false;
					});
			});
			if ((msg.size > 2) && (msg[1]==1),
				{
					/*"got3".postln;*/
					~speed = msg[2];
					~angle = msg[3];
					//"speed".post+~speed.postln;
					//"angle".post+~angle.postln;
					~coef = 0.025; //change this number to adjust the range of the pitch change
					if(~scratch, {
						~pitch_synth.set(\pitch, ~speed*~coef);
						/*"insideOSCdef".postln;
						"speed".post+~speed.postln;*/
						if (((~speed >= 5) || (~speed <= (-5))),
							{~gain_synth.set(\gain, 1)},
							{~gain_synth.set(\gain, 0)}
						);
					});
			});

		},
		'/domino', recvPort: 8888);

	s.sync;
	/*	a.map(\pitch, ~pitch_bus, \mul, ~gain_bus);*/

};
)

~pitch_bus.scope();
~gain_bus.scope();

