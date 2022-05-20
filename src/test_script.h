#pragma once

void LoadTestScript(GameManager& g) {
    // Camera
    Entity* camera = g._entityManager->AddEntity();
    {
        camera->_name = "camera";

        auto pTrans = std::make_unique<TransformComponent>();
        float angle = 45.f * kDeg2Rad;
        Vec3 dir(0.f, sin(angle), cos(angle));
        float dist = 15.f;
        pTrans->SetPos(dist * dir);
        Mat3 rot = Mat3::FromAxisAngle(Vec3(1.f, 0.f, 0.f), -angle);
        pTrans->SetRot(rot);

        auto pCamera = std::make_unique<CameraComponent>();

        camera->_components.push_back(std::move(pTrans));
        camera->_components.push_back(std::move(pCamera));
        camera->ConnectComponents(g);
    }

    // Light
    Entity* light = g._entityManager->AddEntity();
    {
        light->_name = "light";

        auto pTrans = std::make_unique<TransformComponent>();
        pTrans->SetPos(Vec3(0.f, 3.f, 0.f));

        auto pLight = std::make_unique<LightComponent>();
        pLight->_ambient.Set(0.2f, 0.2f, 0.2f);
        pLight->_diffuse.Set(1.f, 1.f, 1.f);

        light->_components.push_back(std::move(pTrans));
        light->_components.push_back(std::move(pLight));
        light->ConnectComponents(g);
    }

    // Cube1
    Entity* cube1 = g._entityManager->AddEntity();
    {
        cube1->_name = "cube1";
        auto tComp = std::make_unique<TransformComponent>();
        auto modelComp = std::make_unique<ModelComponent>();
        modelComp->_modelId = std::string("wood_box");
        TransformComponent* t = tComp.get();
        auto beepComp = std::make_unique<BeepOnHitComponent>();
        beepComp->_synthChannel = 0;
        beepComp->_midiNotes = { 69, 73 };
        auto rbComp = std::make_unique<RigidBodyComponent>();
        rbComp->_localAabb = MakeCubeAabb(0.5f);
        auto velComp = std::make_unique<VelocityComponent>();
        velComp->_angularY = 2*kPi;

        cube1->_components.push_back(std::move(tComp));
        cube1->_components.push_back(std::move(modelComp));
        cube1->_components.push_back(std::move(beepComp));
        cube1->_components.push_back(std::move(rbComp));
        cube1->_components.push_back(std::move(velComp));
        cube1->ConnectComponents(g);
    }

    // Cube2
    Entity* cube2 = g._entityManager->AddEntity();
    cube2->_name = "cube2";
    {
        auto tComp = std::make_unique<TransformComponent>();
        auto modelComp = std::make_unique<ModelComponent>();
        modelComp->_modelId = std::string("wood_box");
        TransformComponent* t = tComp.get();
        t->_transform.Translate(Vec3(-2.f,0.f,0.f));
        // auto rbComp = std::make_unique<RigidBodyComponent>(t, &collisionManager, MakeCubeAabb(0.5f));
        auto rbComp = std::make_unique<RigidBodyComponent>();
        rbComp->_localAabb = MakeCubeAabb(0.5f);
        rbComp->_static = false;
        auto controller = std::make_unique<PlayerControllerComponent>();

        cube2->_components.push_back(std::move(tComp));
        cube2->_components.push_back(std::move(modelComp));
        cube2->_components.push_back(std::move(controller));
        cube2->_components.push_back(std::move(rbComp));
        cube2->ConnectComponents(g);
    }

    // drone sequencer
    Entity* droneSeq = g._entityManager->AddEntity();
    {
        droneSeq->_name = "drone_seq";
        auto seqComp = std::make_unique<SequencerComponent>();
        {
            audio::Event e;
            e.channel = 1;
            e.type = audio::EventType::NoteOn;
            e.midiNote = 45;
            e.timeInTicks = 0;
            seqComp->AddToSequence(e);

            e.midiNote = 49;
            seqComp->AddToSequence(e);

            e.midiNote = 52;
            seqComp->AddToSequence(e);

            e.type = audio::EventType::SynthParam;
            e.param = audio::SynthParamType::Gain;
            e.newParamValue = 0.5;
            seqComp->AddToSequence(e);
        }
        droneSeq->_components.push_back(std::move(seqComp));
        droneSeq->ConnectComponents(g);
    }

    // Init event queue with a synth sequence
    {
        // double const audioTime = Pa_GetStreamTime(audioContext._stream);
        // InitEventQueueWithSequence(&audioContext._eventQueue, beatClock);
        // InitEventQueueWithParamSequence(&audioContext._eventQueue, beatClock);
    }

}